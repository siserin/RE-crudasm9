// makecpu.cpp - Copyright (C) 2010-2012,2014 Willow Schlanger. All rights reserved.
//
// Commands:
//    g++ -o makecpu.out makecpu.cpp
//    ./makecpu.out ../generated
//
// This will regenerate the files in the ../generated directory by processing the
// 'x86iset.ax' file in the current directory.

#include "axiom_lang.h"
#include "axiom_parser.h"
#include "axiom_eval.h"

#include <vector>

#include <cassert>
#include <fstream>
#include <stddef.h>

#include <fstream>
#include <string>
#include <iostream>
using namespace AxiomLanguage;

#include <cstdio>

typedef size_t UINT;

void doPause()
{
#ifdef _MSC_VER
	std::cout << "Press Enter to continue: " << std::flush;
	std::string s;
	std::getline(std::cin, s);
#endif
}

namespace AxiomLanguage
{

//===============================================================================================//

struct EncodingInfoT
{
	U4 opcode1;
	U4 opcode2;
	U4 op66;
	U4 fx;
	U4 regop;
	U4 mod;
	U4 rm;
};

struct InsnInfoT
{
	U4 first_encoding;
	U4 last_encoding;
};

class IntelDecoderTableBuilder
{
	AxiomItemCpuArch *x86;
	size_t etag_reg_base;
	size_t etag_like_arpl;
	size_t etag_like_movsxd;
public:
	std::map<AxiomCpuEncoding *, EncodingInfoT> info;
	std::map<AxiomCpuInsn *, InsnInfoT> insn_info;

	IntelDecoderTableBuilder(AxiomItemCpuArch *x86T)
	{
		x86 = x86T;

		etag_reg_base = 0;
		for(std::map<AxiomItem, size_t>::iterator i = x86->etags.begin(); i != x86->etags.end(); ++i)
		{
			AxiomItem item = i->first;
			if(item->getIdentifier()->getName() == "etag_reg_base")
				etag_reg_base = i->second;
			else
			if(item->getIdentifier()->getName() == "etag_like_arpl")
				etag_like_arpl = i->second;
			else
			if(item->getIdentifier()->getName() == "etag_like_movsxd")
				etag_like_movsxd = i->second;
		}
	}

	std::string buildTable(std::vector<U4> &table);
	std::string process_decoder_encoding(AxiomCpuEncoding &enc, class OpcodeTableContainer *opcon, std::string insn_name);
	std::string process_decoder_modrm(class OpcodeTable *table, size_t offset, AxiomCpuEncoding &enc, class OpcodeTableContainer *opcon, std::string insn_name, U4 enc_regop, U4 enc_mod, U4 enc_rm, U4 opcode1, U4 opcode2);
	std::string process_decoder_entry(U4 enc_regop, U4 enc_mod, U4 enc_rm, U4 opcode1, U4 opcode2, class OpcodeTable *table, size_t offset, AxiomCpuEncoding &enc, class OpcodeTableContainer *opcon, std::string insn_name, int ext_index, int md_index, int size_override = -1);
};

//===============================================================================================//

std::string int_to_string(int x)
{
	char s[33];
	using namespace std;
	sprintf(s, "%d", x);
	return std::string(s);
}

/*
Table codes:
1 - a. 256 entry decoder (full opcode bytes)
2 - b. prefix decoder (replock & op66 splitter)
3 - c. modr/m, high 5 bits - not compatible with (d)
4 - d. modr/m, bits 5..3 - not compatible with (c)
5 - e. modr/m, bits 2..0
6 - f. dsz == 64 bit mode (used by arpl/movsxd)
       not compatible with (g); must be at end if used
7 - g. 256 entry decoder, imm8 after modr/m (3DNow!)
       not compatible with (f); must be at end if used
*/

class OpcodeTable
{
public:
	int type;
	UINT size;
	OpcodeTable **values;		// not used if type == 0
	int encoding_number;		// manually set if type == 0

	OpcodeTable(int typeT)
	{
		type = typeT;

		if(type == 0)
		{
			size = 0;
			values = NULL;
			return;
		}

		switch(type)
		{
		case 2:
			size = 6;
			break;
		case 3:
			size = 32;
			break;
		case 4:
		case 5:
			size = 8;
			break;
		case 6:
			size = 2;
			break;
		default:
			size = 256;
			break;
		}
		values = new OpcodeTable * [size];
		for(size_t i = 0; i < size; ++i)
		{
			values[i] = NULL;
		}
	}
	
	void unalloc()
	{
		if(type == 0)
			return;
		if(values != NULL)
			delete [] values;
		values = NULL;
	}

	OpcodeTable **get(UINT index)
	{
		assert(index < size);
		if(index >= size)
			return NULL;
		return &values[index];
	}

	bool is_level_3_really_level_4()
	{
		if(type != 3 || size != 32)
			return false;
		for(int i = 0; i < 8; ++i)
		{
			OpcodeTable *x = values[i];
			for(int j = 1; j < 4; ++j)
			{
				if(values[i+8*j] != x)
				{
					return false;
				}
			}
		}
		return true;
	}
};

class OpcodeTableContainer
{
public:
	std::list<OpcodeTable *> oplst;
	OpcodeTable *root;

	OpcodeTableContainer()
	{
		root = alloc(1);
	}

	OpcodeTable *alloc(int type)
	{
		oplst.push_back(new OpcodeTable(type));
		return oplst.back();
	}

	~OpcodeTableContainer()
	{
		for(std::list<OpcodeTable *>::iterator i = oplst.begin(); i != oplst.end(); ++i)
		{
			(*i)->unalloc();
			delete *i;
		}
	}
};

struct OpcodeList
{
	std::list<OpcodeTable *> lst;

	OpcodeList(OpcodeTable &root)
	{
		process(&root);
	}

private:
	std::set<OpcodeTable *> visited;
	void process(OpcodeTable *entry)
	{
		if(entry->type == 0 || entry->size <= 0)
			return;		// don't process encoding pointers
		if(visited.find(entry) != visited.end())
			return;		// already been here
		visited.insert(entry);
		for(size_t i = 0; i < entry->size; ++i)
		{
			if(entry->values[i] != NULL)
			{
				process(entry->values[i]);
			}
		}
		lst.push_back(entry);
	}
};

struct Tokens
{
	std::vector<std::string> tokens;
	Tokens(std::string s)
	{
		s += " ";
		std::string tmp;
		for(std::string::iterator i = s.begin(); i != s.end(); ++i)
		{
			if(std::isspace(*i))
			{
				tokens.push_back(tmp);
				tmp.clear();
			}
			else
				tmp += *i;
		}
	}
};

bool is_valid_hex(std::string s)
{
	if(s.empty() || s.size() > 2)
		return false;
	for(size_t i = 0; i < s.size(); ++i)
	{
		char c = s[i];
		if(!((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') || (c >= '0' && c <= '9')))
			return false;
	}
	return true;
}

U1 gethexdigit(char c)
{
	if(c >= '0' && c <= '9')
		return c - '0';
	if(c >= 'a' && c <= 'f')
		return c - 'a' + 0xa;
	if(c >= 'A' && c <= 'F')
		return c - 'A' + 0xa;
	return 16;	// error
}

std::string IntelDecoderTableBuilder::process_decoder_entry(U4 enc_regop, U4 enc_mod, U4 enc_rmT, U4 enc_opcode1, U4 enc_opcode2, class OpcodeTable *table, size_t offset, AxiomCpuEncoding &enc, class OpcodeTableContainer *opcon, std::string insn_name, int ext_index, int md_index, int size_override /*= -1*/)
{
	int extcode;					// -2=/r, -1=none, else 0..7
	if(enc_regop == 8)
		extcode = -2;
	else
	if(enc_regop == 0xf)
		extcode = -1;
	else
		extcode = enc_regop;

	int md;							// -1=default, -2=0..2 claimed, else 0..3
	if(enc_mod == 4)
		md = -2;
	else
	if(enc_mod == 7)
		md = -1;
	else
		md = enc_mod;

	int entry_size = 0;
	if(extcode != -1)
		entry_size = 8;
	if(md != -1)
	{
		if(extcode == -1)
		{
			//std::cout << std::hex << enc.mod << std::dec << std::endl;
			return std::string("Error: An encoding was found with no /0..7 or /r, yet it has a memory type specifier: Instruction ") + insn_name;
		}
		entry_size = 32;
	}
	if(size_override != -1)
		entry_size = size_override;

	if(entry_size == 8)
	{
		for(int md_i = 0; md_i < 4; ++md_i)
		{
			std::string result = process_decoder_entry(enc_regop, enc_mod, enc_rmT, enc_opcode1, enc_opcode2, table, offset, enc, opcon, insn_name, ext_index, md_i, 32);
			if(!result.empty())
				return result;
		}

		return "";
	}

		OpcodeTable **ptr = table->get(offset);

	if(entry_size == 0)
	{
		if(*ptr != NULL && (*ptr)->type <= 4)
		{
			return std::string("Opcode conflict, line ") + int_to_string(__LINE__) + std::string(", instruction: ") + insn_name;
		}
	}
	else
	{
		// entry_size must be 32
		if(*ptr != NULL && (*ptr)->type != 3)
		{
			return std::string("Opcode conflict, line ") + int_to_string(__LINE__) + std::string(", instruction: ") + insn_name;
		}

		if(*ptr == NULL)
		{
			*ptr = opcon->alloc(3);
		}
		table = *ptr;
		offset = ext_index + 8 * md_index;

		if(size_override == 32 && md_index != 0)
		{
			ptr = table->get(offset);
			if(*ptr != NULL && *ptr != *table->get(ext_index))
			{
				return std::string("Opcode conflict, line ") + int_to_string(__LINE__) + std::string(", instruction: ") + insn_name;
			}
			*ptr = *table->get(ext_index);
			return "";
		}
	}

	int enc_rm;
	if(enc_rmT == 0xf)
		enc_rm = -1;
	else
		enc_rm = enc_rmT;

	ptr = table->get(offset);
	if(enc_rm != -1)
	{
		if(*ptr != NULL && (*ptr)->type != 5)
		{
			return std::string("Opcode conflict, line ") + int_to_string(__LINE__) + std::string(", instruction: ") + insn_name;
		}

		if(*ptr == NULL)
		{
			*ptr = opcon->alloc(5);
		}
		table = *ptr;
		offset = static_cast<size_t>(enc_rm);
	}
	else
	{
		if(*ptr != NULL && (*ptr)->type <= 5)
		{
			return std::string("Opcode conflict, line ") + int_to_string(__LINE__) + std::string(", instruction: ") + insn_name;
		}
	}

	ptr = table->get(offset);

	if((etag_like_arpl != 0 && (enc.etags & (etag_like_arpl)) != 0) || (etag_like_movsxd != 0 && (enc.etags & (etag_like_movsxd)) != 0))
	{
		if(*ptr != NULL && (*ptr)->type != 6)
		{
			return std::string("Opcode conflict, line ") + int_to_string(__LINE__) + std::string(", instruction: ") + insn_name;
		}

		if(*ptr == NULL)
		{
			*ptr = opcon->alloc(6);
		}
		table = *ptr;
		offset = ((etag_like_movsxd != 0 && (enc.etags & (etag_like_movsxd)) != 0)) ? 1 : 0;
	}

	ptr = table->get(offset);
	if(enc_opcode1 >= 0x200)
	{
		// 3DNow! instruction of type that has imm8 opcode after modr/m.
		if(*ptr != NULL && (*ptr)->type != 7)
		{
			return std::string("Opcode conflict, line ") + int_to_string(__LINE__) + std::string(", instruction: ") + insn_name;
		}

		if(*ptr == NULL)
		{
			*ptr = opcon->alloc(7);
		}
		table = *ptr;
		offset = enc_opcode2;
		if(offset < 0 || offset >= 256)
		{
			return std::string("Error, line ") + int_to_string(__LINE__) + std::string(", instruction: ") + insn_name;
		}
	}

	ptr = table->get(offset);

	if(*ptr != NULL)
	{
		return std::string("Duplicate opcode definition, instruction: ") + insn_name;
	}

	*ptr = opcon->alloc(0);
	(*ptr)->encoding_number = enc.encoding_num;

	if(insn_info.find(enc.insn) == insn_info.end())
	{
		insn_info[enc.insn].first_encoding = enc.encoding_num;
		insn_info[enc.insn].last_encoding = enc.encoding_num;
	}
	else
	{
		if(enc.encoding_num < insn_info[enc.insn].first_encoding)
			insn_info[enc.insn].first_encoding = enc.encoding_num;
		if(enc.encoding_num > insn_info[enc.insn].last_encoding)
			insn_info[enc.insn].last_encoding = enc.encoding_num;
	}

	return "";
}

std::string IntelDecoderTableBuilder::process_decoder_modrm(class OpcodeTable *table, size_t offset, AxiomCpuEncoding &enc, class OpcodeTableContainer *opcon, std::string insn_name,
	U4 enc_regop, U4 enc_mod, U4 enc_rm, U4 enc_opcode1, U4 enc_opcode2
)
{
	int extcode;					// -2=/r, -1=none, else 0..7
	if(enc_regop == 8)
		extcode = -2;
	else
	if(enc_regop == 0xf)
		extcode = -1;
	else
		extcode = enc_regop;

	int md;							// -1=default, -2=0..2 claimed, else 0..3
	if(enc_mod == 4)
		md = -2;
	else
	if(enc_mod == 7)
		md = -1;
	else
		md = enc_mod;

	int ext_begin = 0, ext_stop = 0;
	if(extcode >= 0)
	{
		ext_begin = ext_stop = extcode;
	}
	else
	if(extcode == -2)
	{
		ext_begin = 0;
		ext_stop = 7;
	}

	int md_begin = 0, md_stop = 0;
	if(md == -2)
	{
		md_begin = 0;
		md_stop = 2;
	}
	else
	if(md >= 0)
	{
		md_begin = md_stop = md;
	}

	for(int ext_index = ext_begin; ext_index <= ext_stop; ++ext_index)
	{
		for(int md_index = md_begin; md_index <= md_stop; ++md_index)
		{
			std::string result = process_decoder_entry(enc_regop, enc_mod, enc_rm, enc_opcode1, enc_opcode2, table, offset, enc, opcon, insn_name, ext_index, md_index);
			if(!result.empty())
				return result;
		}
	}

	return "";
}

std::string IntelDecoderTableBuilder::process_decoder_encoding(AxiomCpuEncoding &enc, class OpcodeTableContainer *opcon, std::string insn_name)
{
	OpcodeTable *root = opcon->root;
	OpcodeTable *table = root;

	Tokens opcodeTokens(enc.opcodes);
	if(opcodeTokens.tokens.empty())
		return "invalid empty opcode string";

	bool didOpcode1 = false;
	U4 opcode1 = 0;		// none yet
	U4 opcode2 = 0x100;		// means none

	U4 op66 = 0;		// 0=default, 1=no66, 2=66
	U4 fx = 1;			// 0=nofx, 1=default, 2=f2, 3=f3
	U4 regop = 0xf;		// 0..7, 8=/r, f=none
	U4 mod = 7;			// 0..3, 4={0,1,2} (i.e. /mod_m), 7=none
	U4 rm = 0xf;		// 0..7, f=none

	for(size_t i = 0; i < opcodeTokens.tokens.size(); ++i)
	{
		std::string s = opcodeTokens.tokens[i];
		if(s.empty())
			continue;	// shouldn't happen
		if(s == "nofx")
			fx = 0;
		else
		if(s == "no66")
			op66 = 1;
		else
		if(s[0] == '&')
		{
			s = std::string(++s.begin(), s.end());
			if(!is_valid_hex(s))
				return std::string("invalid hex opcode: ") + s;
			if(opcode2 != 0x100)
				return "too many additional opcode bytes";
			opcode2 = gethexdigit(s[0]);
			if(s.size() > 1)
			{
				opcode2 *= 16;
				opcode2 += gethexdigit(s[1]);
			}
		}
		else
		if(is_valid_hex(s))
		{
			U1 x = gethexdigit(s[0]);
			if(s.size() > 1)
			{
				x *= 16;
				x += gethexdigit(s[1]);
			}
			if(x == 0x0f)
			{
				if(didOpcode1)
					return "too many opcode bytes [2]";
				if(opcode1 == 0)
					opcode1 = 0x100;
				else
				if(opcode1 == 0x100)
					opcode1 = 0x200;
				else
				{
					return "invalid use of 0f prefix";
				}
			}
			else
			if(x == 0xf2)
				fx = 2;
			else
			if(x == 0xf3)
				fx = 3;
			else
			if(x == 0x66)
				op66 = 2;
			else
			{
				if(didOpcode1)
					return "too many opcode bytes";
				opcode1 += x;
				didOpcode1 = true;
			}
		}
		else
		if(s == "/r")
			regop = 8;
		else
		if(s.size() == 2 && s[0] == '/' && s[1] >= '0' && s[1] <= '7')
			regop = s[1] - '0';
		else
		if(s == "/mod_m")
			mod = 4;
		else
		if(s == "/mod_0")
			mod = 0;
		else
		if(s == "/mod_1")
			mod = 1;
		else
		if(s == "/mod_2")
			mod = 2;
		else
		if(s == "/mod_3")
			mod = 3;
		else
		if(s == "/rm_0")
			rm = 0;
		else
		if(s == "/rm_1")
			rm = 1;
		else
		if(s == "/rm_2")
			rm = 2;
		else
		if(s == "/rm_3")
			rm = 3;
		else
		if(s == "/rm_4")
			rm = 4;
		else
		if(s == "/rm_5")
			rm = 5;
		else
		if(s == "/rm_6")
			rm = 6;
		else
		if(s == "/rm_7")
			rm = 7;
		else
		{
			return std::string("invalid opcode specifier: ") + s;
		}
	}

	info[&enc].opcode1 = opcode1;
	info[&enc].opcode2 = opcode2;
	info[&enc].op66 = op66;
	info[&enc].fx = fx;
	info[&enc].regop = regop;
	info[&enc].mod = mod;
	info[&enc].rm = rm;

	U4 opcode_3dnow = 0x100;

	size_t opcode_count = 0;
	U1 opcodes[3];

	if(opcode1 < 0x100)
	{
		opcodes[0] = opcode1;
		opcode_count = 1;

		if(opcode2 != 0x100)
		{
			opcodes[1] = opcode2;
			opcode_count = 2;
		}
	}
	else
	if(opcode1 < 0x200)
	{
		opcodes[0] = 0x0f;
		opcodes[1] = opcode1 & 0xff;
		opcode_count = 2;

		if(opcode2 != 0x100)
		{
			opcodes[2] = opcode2;
			opcode_count = 3;
		}
	}
	else
	{
		// 3DNow! Instruction
		opcode_3dnow = opcode2;
		opcode2 = 0x100;
		opcodes[0] = 0x0f;
		opcodes[1] = 0x0f;
		opcodes[2] = opcode1 & 0xff;
	}

	size_t offset;
	for(size_t i = 0; ;)
	{
		if(table->type != 1)
		{
			return std::string("Internal error with ") + insn_name + std::string(", code ") + int_to_string(__LINE__);
		}

		offset = opcodes[i];
		++i;
		if(i == opcode_count)
			break;
		OpcodeTable **ptr = table->get(offset);
		if(*ptr == NULL)
			*ptr = opcon->alloc(1);
		table = *ptr;
	}

	size_t old_offset = offset;
	size_t count = 1;

	if(etag_reg_base != 0 && (enc.etags & (etag_reg_base)) != 0)
	{
		count = 8;
	}

	OpcodeTable *old_table = table;
	for(size_t qqq = 0; qqq < count; ++qqq)
	{
		size_t offset = old_offset + qqq;
		OpcodeTable *table = old_table;

		// Prefix bytes.
		if(op66 == 0 && fx == 1)
		{
			OpcodeTable **ptr = table->get(offset);
			if(*ptr != NULL && (*ptr)->type <= 2)
			{
				return std::string("Internal error with ") + insn_name + std::string(", code ") + int_to_string(__LINE__);
			}

			std::string result = process_decoder_modrm(table, offset, enc, opcon, insn_name, regop, mod, rm, opcode1, opcode2);
			if(!result.empty())
				return result;
		}
		else
		{
			OpcodeTable **ptr = table->get(offset);
			if(*ptr == NULL)
				*ptr = opcon->alloc(2);
			else
			if((*ptr)->type != 2)
			{
				return std::string("Internal error with ") + insn_name + std::string(", code ") + int_to_string(__LINE__);
			}
			table = *ptr;	// prefix table
		
			// 66 : (no66, 66)     : 0..1
			// fx : (nofx, f2, f3) : 0..2
			size_t num66 = (op66 == 0) ? 2 : 1;
			size_t start66 = (op66 == 2) ? 1 : 0;
			size_t numfx = (fx == 1) ? 3 : 1;
			size_t startfx = 0;
			if(fx == 2)
				startfx = 1;
			else
			if(fx == 3)
				startfx = 2;

			for(size_t a = 0; a < num66; ++a)
			{
				for(size_t b = 0; b < numfx; ++b)
				{
					offset = (start66 + a) + 2 * (startfx + b);

					std::string result = process_decoder_modrm(table, offset, enc, opcon, insn_name, regop, mod, rm, opcode1, opcode2);
					if(!result.empty())
						return result;
				}
			}
		}
	}

	return "";	// success
}

std::string IntelDecoderTableBuilder::buildTable(std::vector<U4> &table)
{
	table.clear();
	OpcodeTableContainer opcon;

	for(std::map<std::string, AxiomCpuInsn>::iterator i = x86->cpuInsns.begin(); i != x86->cpuInsns.end(); ++i)
	{
		//std::cout << syntax->getSymbol(i->proto.name) << std::endl;
		for(std::list<AxiomCpuEncoding>::iterator j = i->second.encodings.begin(); j != i->second.encodings.end(); ++j)
		{
			std::string result = process_decoder_encoding(*j, &opcon, i->first);
			if(!result.empty())
				return std::string("Error building decoder table for instruction: ") + i->first + std::string(": ") + result;
		}
	}

	OpcodeList ol(*opcon.root);
	U4 opcode_root = 0;
	std::vector<U4> &opcodes = table;

	std::map<OpcodeTable *, U4> m;
	for(std::list<OpcodeTable *>::iterator i = ol.lst.begin(); i != ol.lst.end(); ++i)
	{
		OpcodeTable *table = *i;
		if(table == opcon.root)
		{
			opcode_root = (1 << 24) | static_cast<U4>(opcodes.size());
		}
		m[table] = static_cast<U4>(opcodes.size());	// remember this table

		UINT table_size = table->size;

		if(table->is_level_3_really_level_4())
		{
			table_size = 8;
			///std::cout  << "+" << std::endl;
		}

		for(size_t j = 0; j < table_size; ++j)
		{
			OpcodeTable *entry = table->values[j];
			if(entry == NULL)
				opcodes.push_back(0x00ffffff);		// invalid opcode
			else
			if(entry->type == 0)
				opcodes.push_back(entry->encoding_number);
			else
			{
				assert(m.find(entry) != m.end());	// make sure we already processed this table
				int entry_type = entry->type;
				if(entry->is_level_3_really_level_4())
					entry_type = 4;
				opcodes.push_back((entry_type << 24) | m[entry]);
			}
		}
	}

	table.push_back(opcode_root);
	return "";	// success
}

}	// namespace AxiomLanguage

using namespace AxiomLanguage;

int main(int argc, char **argv)
{
	if(argc != 2)
	{
		std::cout << "Copyright (C) 2010-2012,2014 Willow Schlanger. All rights reserved." << std::endl;
		std::cout << "Usage: makecpu <output path>" << std::endl;
		std::cout << "Note: x86iset.ax must be in the current directory when this is run!" << std::endl;
		doPause();
		return 1;
	}
	std::string path = argv[1];
	for(std::string::iterator i = path.begin(); i != path.end(); ++i)
	{
		if(*i == '\\')
			*i = '/';
	}
	if(!path.empty() && path[path.size()-1] != '/')
		path += "/";
	
	using namespace std;
	FILE *afi = fopen("x86iset.ax", "rb");
	if(afi == NULL)
	{
		std::cout << "Error: unable to open file for reading: x86iset.ax" << std::endl;
		doPause();
		return 1;
	}
	fclose(afi);
	afi = NULL;

	try
	{
		AxiomState *aState = new AxiomState();
		AxiomParser<std::string::iterator> parser(aState);
		AxiomEvaluator eval(aState, std::cerr);
		{
			AxiomItem cpu = parser.parseIncludeFile("x86iset.ax", std::cerr);
			if(cpu.isNull())
			{
				cpu.clear();
				delete aState;
				doPause();
				return 1;
			}
			
			IntelDecoderTableBuilder builder(cpu->getCpuArch());
			std::vector<U4> table;
			std::cout << "Working..." << std::endl;
			std::string status = builder.buildTable(table);
	
			if(status != "")
			{
				cpu.clear();
				std::cout << "Error: " << status << std::endl;
				delete aState;
				doPause();
				return 1;
			}
	
			if(table.size() < 2)
			{
				cpu.clear();
				std::cout << "Internal error -- no table generated!" << std::endl;
				delete aState;
				doPause();
				return 1;
			}

			{
				std::ofstream fo((path + "out_intel_decoder_table.h").c_str());
				if(fo == NULL)
				{
					std::cout << "Error: unable to create file: ../../generated/out_intel_decoder_table.h" << std::endl;
					cpu.clear();
					delete aState;
					doPause();
					return 1;
				}
				fo << "// out_intel_decoder_table.h  (note: this file was automatically generated -- do not edit!)\n";
				fo << "// Copyright (C) 2011,2012 Willow Schlanger. All rights reserved.\n";
				fo << "\nU4 crudasm_intel_decoder_table[] = {";
				for(size_t i = 0; i < table.size(); ++i)
				{
					if(i != 0)
						fo << ",";
					if(i % 16 == 0)
					{
						fo << "\n\t";
					}
					if(i == 0)
						fo << "0x" << std::hex << table[table.size() - 1] << std::dec;
					else
						fo << "0x" << std::hex << table[i - 1] << std::dec;
				}
				fo << "\n";
				fo << "};\n";
				fo << std::endl;
			}

			{
				std::ofstream fo((path + "out_intel_encoding_table.h").c_str());
				if(fo == NULL)
				{
					std::cout << "Error: unable to create file: " << path << "out_intel_encoding_table.h" << std::endl;
					cpu.clear();
					delete aState;
					doPause();
					return 1;
				}
				fo << "// out_intel_encoding_table.h  (note: this file was automatically generated -- do not edit!)\n";
				fo << "// Copyright (C) 2011,2012 Willow Schlanger. All rights reserved.\n";
				fo << "\nstruct crudasm_intel_encoding_t crudasm_intel_encoding_table[] = {\n";
		
				AxiomItemCpuArch *x86 = cpu->getCpuArch();
				size_t numEncodings = x86->getNumEncodings();
				for(size_t i = 0; i < numEncodings; ++i)
				{
					if(i != 0)
						fo << ",\n";
					fo << "\t";
					AxiomCpuEncoding *encoding = x86->getEncoding(i);
					EncodingInfoT *einfo = &(builder.info[encoding]);

					fo << "{";
					fo << std::hex;

					fo << "crudasm_intel_insn_" << *encoding->insn->alias << ", ";
					fo << "0x" << einfo->op66 << ", ";
					fo << "0x" << einfo->opcode1 << ", ";
					fo << "0x" << einfo->opcode2 << ", ";
					fo << "0x" << einfo->mod << ", ";
					fo << "0x" << einfo->regop << ", ";
					fo << "0x" << einfo->rm << ", ";
					fo << "0x" << einfo->fx << ", ";

					fo << "{";
					for(int i = 0; i < AXIOM_MAX_ASM_ARGS; ++i)
					{
						if(i != 0)
							fo << ",";
						if(encoding->arg_types[i].isNull())
							fo << "0";
						else
							fo << "crudasm_intel_argtype_" << encoding->arg_types[i]->getIdentifier()->getName();
					}
					fo << "},";

					fo << "{";
					for(int i = 0; i < AXIOM_MAX_ASM_ARGS; ++i)
					{
						if(i != 0)
							fo << ",";
						if(encoding->arg_sizes[i].isNull())
							fo << "0";
						else
							fo << "crudasm_intel_argsize_" << encoding->arg_sizes[i]->getIdentifier()->getName();
					}
					fo << "},";

					fo << "{";
					for(int i = 0; i < AXIOM_MAX_ASM_ARGS; ++i)
					{
						if(i != 0)
							fo << ",";
						if(encoding->arg_values[i].isNull())
							fo << "crudasm_intel_argvalue_default";
						else
						if(encoding->arg_values[i]->getInteger() != NULL)
							fo << "0x" << encoding->arg_values[i]->getInteger()->getScalarValue();
						else
							fo << "crudasm_intel_argvalue_" << encoding->arg_values[i]->getIdentifier()->getName();
					}
					fo << "},";

					fo << "0";
					for(std::map<AxiomItem, size_t>::iterator i = x86->etags.begin(); i != x86->etags.end(); ++i)
					{
						AxiomItem f = i->first;
						if((encoding->etags & i->second) != 0)
							fo << "|crudasm_intel_" << f->getIdentifier()->getName();
					}

					fo << std::dec << "}";
				}
				fo << ",\n";
				fo << "\t{crudasm_intel_insncount}\n";
				fo << "};\n";
				fo << std::endl;

				std::ofstream fo2((path + "out_intel_encoding_info.h").c_str());
				if(fo2 == NULL)
				{
					std::cout << "Error: unable to create file: " << path << "out_intel_encoding_info.h" << std::endl;
					cpu.clear();
					delete aState;
					doPause();
					return 1;
				}
				fo2 << "// out_intel_encoding_info.h  (note: this file was automatically generated -- do not edit!)\n";
				fo2 << "// Copyright (C) 2011,2012 Willow Schlanger. All rights reserved.\n";

				fo2 << "\nenum { CRUDASM_MAX_ASM_ARGS = " << AXIOM_MAX_ASM_ARGS << " };\n";
		
				fo2 << "\n";
				fo2 << "struct crudasm_intel_encoding_t {\n";
				fo2 << "\tU4 insn : 30;\t// instruction number (index)\n";
				fo2 << "\tU4 op66 : 2;\t// 0=default, 1=no66, 2=66\n";
				fo2 << "\t// opcode1 : 100..1ff = 0f <low 8 bits>\n";
				fo2 << "\tU4 opcode1 : 10;\t// 0..2ff (200..2ff = 3DNow!, opcode 2 is the 3DNow! imm8 opcode after modr/m)\n";
				fo2 << "\tU4 opcode2 : 9;\t// 0..ff, 100 for none\n";
				fo2 << "\tU4 mod : 3;\t// 0..3, 4={0,1,2} (i.e. /mdm), 7=none\n";
				fo2 << "\tU4 regop : 4;\t// 0..7, 8=/r, f=none\n";
				fo2 << "\tU4 rm : 4;\t// 0..7, f=none\n";
				fo2 << "\tU4 fx : 2;\t// 0=nofx, 1=default, 2=f2, 3=f3\n";
				fo2 << "\tU1 argtype[CRUDASM_MAX_ASM_ARGS];\t// 0 for void\n";
				fo2 << "\tS1 argsize[CRUDASM_MAX_ASM_ARGS];\t// 1..127 bytes; 0 for void. If <0, see enumeration\n";
				fo2 << "\tU1 argvalue[CRUDASM_MAX_ASM_ARGS];\t// usually 0..127; default value is 0x80, 0x81 means reg_or_mem\n";
				fo2 << "\tU4 etags;\t// encoding tags\n";
				fo2 << "};\n";
				fo2 << "\n#ifdef __cplusplus\n";
				fo2 << "extern \"C\" {\n";
				fo2 << "#endif\n";
				fo2 << "\nextern struct crudasm_intel_encoding_t crudasm_intel_encoding_table[];  /* see out_intel_encoding_table.h */\n";
				fo2 << "\nextern U4 crudasm_intel_decoder_table[];  /* see out_intel_decoder_table.h */\n";
				fo2 << "\n#ifdef __cplusplus\n";
				fo2 << "}\n";
				fo2 << "#endif\n";

				fo2 << "\n";
				fo2 << "enum {\n";
				for(std::map<std::string, AxiomCpuInsn>::iterator i = x86->cpuInsns.begin(); i != x86->cpuInsns.end(); ++i)
				{
					fo2 << "\tcrudasm_intel_insn_" << i->first << ",\n";
				}
				fo2 << "\tcrudasm_intel_insncount\n};\n";

				fo2 << "\nenum {\n";
				for(std::map<AxiomItem, size_t>::iterator i = x86->etags.begin(); i != x86->etags.end(); ++i)
				{
					AxiomItem f = i->first;
					fo2 << "\tcrudasm_intel_" << f->getIdentifier()->getName() << " = 0x" << std::hex << i->second << std::dec << "," << "\n";
				}
				fo2 << "\tcrudasm_intel_etagcount = " << x86->etags.size() << "\n};\n";

				fo2 << "\nenum {\n";
				for(std::map<AxiomItem, size_t>::iterator i = x86->itags.begin(); i != x86->itags.end(); ++i)
				{
					AxiomItem f = i->first;
					fo2 << "\tcrudasm_intel_" << f->getIdentifier()->getName() << " = 0x" << std::hex << i->second << std::dec << "," << "\n";
				}
				fo2 << "\tcrudasm_intel_itagcount = " << x86->itags.size() << "\n};\n";

				fo2 << "\nstruct crudasm_intel_insn_t {\n";
				fo2 << "\tconst char *alias;\n";
				fo2 << "\tU4 itags;\t// instruction tags\n";
				fo2 << "\tU4 valid_modes;\n";
				fo2 << "\tU4 first_encoding_index;\n";
				fo2 << "\tU4 last_encoding_index;\n";
				fo2 << "\tU1 numArgs;\n";
				fo2 << "\tS1 argSizes[CRUDASM_MAX_ASM_ARGS];\n";
				fo2 << "};\n\n";

				fo2 << "\n#ifdef __cplusplus\n";
				fo2 << "extern \"C\" {\n";
				fo2 << "#endif\n";
				fo2 << "\nextern struct crudasm_intel_insn_t crudasm_intel_insns[];  /* see out_intel_encoding_table.h */\n";
				fo2 << "\n#ifdef __cplusplus\n";
				fo2 << "}\n";
				fo2 << "#endif\n";

				fo2 << "enum {\n";
				for(std::list<AxiomItem>::iterator j = x86->coreCpuModesList.begin(); j != x86->coreCpuModesList.end(); ++j)
				{
					AxiomItem x = *j;
					size_t y = x86->cpuModes[x];
					fo2 << "\tcrudasm_intel_mode_" << x->getIdentifier()->getName() << " = 0x" << std::hex << y << std::dec << ",\n";
				}
				fo2 << "\tcrudasm_intel_modecount\n};\n";

				fo2 << std::endl;

				std::ofstream fo3((path + "out_intel_insn_table.h").c_str());
				if(fo3 == NULL)
				{
					std::cerr << "Error: unable to create file: " << path << "out_intel_insn_table.h" << std::endl;
					cpu.clear();
					delete aState;
					doPause();
				}
				fo3 << "// out_intel_insn_table.h  (note: this file was automatically generated -- do not edit!)\n";
				fo3 << "// Copyright (C) 2012 Willow Schlanger. All rights reserved.\n\n";
				fo3 << "struct crudasm_intel_insn_t crudasm_intel_insns[] = {\n";
				for(std::map<std::string, AxiomCpuInsn>::iterator i = x86->cpuInsns.begin(); i != x86->cpuInsns.end(); ++i)
				{
					fo3 << "\t{";
					fo3 << "\"" << *i->second.alias << "\", 0";
					
					for(std::map<AxiomItem, size_t>::iterator j = x86->itags.begin(); j != x86->itags.end(); ++j)
					{
						AxiomItem f = j->first;
						if((i->second.insn_flags & j->second) != 0)
							fo3 << "|crudasm_intel_" << f->getIdentifier()->getName();
					}

					fo3 << ", 0";
					for(std::list<AxiomItem>::iterator j = x86->coreCpuModesList.begin(); j != x86->coreCpuModesList.end(); ++j)
					{
						AxiomItem x = *j;
						size_t y = x86->cpuModes[x];
						if((i->second.valid_modes & y) != 0)
							fo3 << "|crudasm_intel_mode_" << x->getIdentifier()->getName();
					}

					fo3 << ", 0x" << std::hex << builder.insn_info[&(i->second)].first_encoding << std::dec;
					fo3 << ", 0x" << std::hex << builder.insn_info[&(i->second)].last_encoding << std::dec;
					fo3 << ", " << (U2)(i->second.numArgs);
					fo3 << ", ";
					fo3 << "{";
					for(size_t k = 0; k < AXIOM_MAX_ASM_ARGS; ++k)
					{
						if(k != 0)
							fo3 << ",";
						std::string u = i->second.argSizes[k];
						if(u.empty())
							fo3 << "0";
						else
						if(i->second.argParamSize.find(u) != i->second.argParamSize.end())
						{
							int v = i->second.argParamSize[u] + 1;
							fo3 << (int)(-v);
						}
						else
							fo3 << "crudasm_intel_argsize_" << u;
					}
					fo3 << "}";

					fo3 << "},\n";
				}
				fo3 << "\t{0}\n};\n";
				fo3 << std::endl;

				std::ofstream fo4((path + "out_intel_disasm_nasm.h").c_str());
				if(fo4 == NULL)
				{
					std::cerr << "Error: unable to create file: " << path << "out_intel_disasm_nasm.h" << std::endl;
					cpu.clear();
					delete aState;
					doPause();
				}
				fo4 << "// out_intel_disasm_nasm.h  (note: this file was automatically generated -- do not edit!)\n";
				fo4 << "// Copyright (C) 2012 Willow Schlanger. All rights reserved.\n\n";

				fo4 << "static int crudasm_intel_disasm_special_nasm(struct crudasm_intel_disasm_context_t *context, U4 insn) {\n";

				bool gotAny = false;
				for(std::map<std::string, AxiomCpuInsn>::iterator i = x86->cpuInsns.begin(); i != x86->cpuInsns.end(); ++i)
				{
					AxiomCpuInsn insn = i->second;
					if(insn.disasm_special)
					{
						gotAny = true;
						break;
					}
				}

				if(gotAny)
				{
#if 0				// throw-away code to stub disassembler functions
					for(std::map<std::string, AxiomCpuInsn>::iterator i = x86->cpuInsns.begin(); i != x86->cpuInsns.end(); ++i)
					{
						AxiomCpuInsn &insn = i->second;
						if(!insn.disasm_special)
								continue;
						fo4 << "\nint intel_nasm_insn_" << i->first << "(struct crudasm_intel_disasm_context_t *context) {\n";
						fo4 << "\treturn 1;\t// handled\n";
						fo4 << "}\n";
					}
#endif				// end throw-away code

					fo4 << "\tswitch(insn) {\n";
					for(std::map<std::string, AxiomCpuInsn>::iterator i = x86->cpuInsns.begin(); i != x86->cpuInsns.end(); ++i)
					{
						AxiomCpuInsn &insn = i->second;
						if(!insn.disasm_special)
								continue;
						fo4 << "\t\tcase crudasm_intel_insn_" << i->first << ":\n";
						if(insn.disasm_second_name.empty())
							fo4 << "\t\t\treturn intel_nasm_insn_" << i->first << "(context);\n";
						else
						{
							fo4 << "\t\t\t{\n";
							fo4 << "\t\t\t\tixdis1_write(context, \"" << insn.disasm_second_name << "\");\n";
							fo4 << "\t\t\t\tixdis0_write_any_args(context);\n";
							fo4 << "\t\t\t\treturn 1;\t// handled\n";
							fo4 << "\t\t\t}\n";
						}
					}
					fo4 << "\t\tdefault: break;\n";
					fo4 << "\t}\n";
				}

				fo4 << "\treturn 0;\t// not handled specially\n}\n";

				fo4 << std::endl;
			}

		}

		delete aState;
	}
	catch(...)
	{
	}
#if defined(AXIOM_LANGUAGE_DEBUG_LEVEL) && AXIOM_LANGUAGE_DEBUG_LEVEL >= 1
	std::cout << "\n" << numObjectsGlobal << " objects active." << std::endl;
#endif
	doPause();
	return 0;
}
