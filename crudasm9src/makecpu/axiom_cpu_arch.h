// axiom_cpu_arch.h - Copyright (C) 2012 Willow Schlanger. All rights reserved.
// Do not include this file directly: include axiom_lang.h instead.

#ifndef l_crudasm__axiom_cpu_arch_h__included
#define l_crudasm__axiom_cpu_arch_h__included

enum
{
	AXIOM_MAX_ASM_ARGS = 4
};

// return value: BIT -> 0
// B<number> -> number, assuming it doesn't overflow
// anything else returns -1
inline int getAsmSize(std::string s)
{
	if(s == "BIT")
		return 0;
	if(s[0] != 'B' || s.size() < 2)
		return -1;
	int value = 0;
	for(size_t i = 1; i < s.size(); ++i)
	{
		if(s[i] >= '0' && s[i] <= '9')
		{
			int old = value;
			value *= 10;
			if(value < old)
				return -1;	// got overflow!
			value = (old * 10) + (int)(s[i] - '0');
		}
		else
			return -1;
	}
	return value;
}

struct AxiomCpuEncoding
{
	struct AxiomCpuInsn *insn;
	size_t etags;
	size_t encoding_num;
	std::string opcodes;
	AxiomItem arg_types[AXIOM_MAX_ASM_ARGS];
	AxiomItem arg_sizes[AXIOM_MAX_ASM_ARGS];
	AxiomItem arg_values[AXIOM_MAX_ASM_ARGS];		// this may be an identifier or a QWORD integer with value 0..127, or it may be NULL for default value
};

struct AxiomCpuSemanticsNode
{
	// some special opcode's:
	// "$num" --> literal number. size is 'tag' bytes; value stored in value[0] & value[1].
	//            if a number appears in our x86iset.ax source by itself, its size is B8.
	//            we can also do (B2 <number>) to truncate a number to a size of B2, etc.
	//            (B10/B16 number1 number2) are also allowed. the low-order QWORD appears
	//            first, then comes the high-order QWORD.
	// "$arg" --> argument. argument is selected by 'tag', 0 is 1st arg, 1 is 2nd, etc.
	// "$tmp" --> temporary. which temp is selected by 'value[0]', 0 is 1st, 1 is 2nd, etc.
	//            note that alloc's are allowed at global emu scope only--not e.g.
	//            within a times block.
	// "$argsize" -> refers to the size of the argument indicated by 'tag'.
	//               'tag' will be 0 for 1st argsize, 1 for 2nd argsize, etc.
	//               the size of an $argsize node is B8, so it's frequently truncated via zx.
	// if operands is not empty, then 'opcode' refers to a function.
	// here are some built-in functions:
	// _zx        zero-extend or truncate. 'tag' gets the new size; operands gets the src.
	// _sx        sign-extend (or truncate). 'tag' gets the new size; operands gets the src.
	// if operands is empty, and opcode does not begin with a '$', then we refer to an
	// external object, such as ZF or EAX.
	// Some more special functions:
	// (_fetch_stack <B8 size in bytes> <offset>)
	// (_fetch_signed_ofs <argument; this determines size too> <offset>)
	// alloc commands: tag holds size.

	std::string opcode;
	std::list<AxiomCpuSemanticsNode *> operands;
	U8 value[2];
	S1 tag;

	AxiomCpuSemanticsNode()
	{
		tag = 0;
		value[0] = 0;
		value[1] = 0;
	}
};

/*
commands possible:
(alloc <size> <name> [<src>])
	these must be a global emu scope, and in particular cannot be inside an 'if' or 'times' block.
(asgn <dest> <src>)
(push <src>)
(pop <dest>)
(inport <dest> <port-to-read>)
(outport <port-to-write> <src>)
(assert <bit-cond> <except-code-1> <except-code-2> <except-arg>)
	except-code-1: if 0, we simply raise the exception indicated by except-code-2, with except-arg
	being available for error code calculations. all 'codes' here are B8's.
(times (<temp-var-name, previously defined> <start value> <end value (we stop before reaching this value)>
	<# of significant bits, must be a literal (B8)>) ...commands...
	start value & end value must be known at CHECK time.
(if <bit-condition> ...commands...)
	bit-condition must be known at CHECK time
(check_stack_writable <B8 size in bytes, normally 1/2/4/8> <offset; segment is SS>)

What about DIV? This instruction should be marked 'advanced'.
assert's must be at global emu scope. They may be preceded only by alloc's and asgn's to temp's.
The intent is that if an assertion is going to fail, we don't want to have to roll back any changes.
What if we DIV DWORD [EAX] ??? If a D/A bit needs to be set, a page fault will be generated and we'll
emulate the instruction the slow way (this is simpler).
*/
struct AxiomCpuSemantics
{
	size_t numAllocTemps;
	std::list<AxiomCpuSemanticsNode> nodes;			// all nodes (in no particular order)
	std::list<AxiomCpuSemanticsNode *> commands;	// commands, in order
	std::map<std::string, size_t> tempNames;

	static bool isCommand(std::string s)
	{
		if(s == "alloc" ||
			s == "asgn" ||
			s == "push" ||
			s == "pop" ||
			//s == "inport" ||
			//s == "outport" ||
			s == "assert" ||
			s == "check_stack_writable" ||
			s == "store_signed_ofs" ||
			s == "store_stack" ||
			s == "notice" ||					// auto-generated e.g. for non-literal TIMES bounds or IF conditions
			s == "times" ||
			s == "if"
		)
			return true;
		return false;
	}
};

struct AxiomCpuInsn
{
	AxiomItem name;
	const std::string *alias;
	size_t insn_flags;
	size_t valid_modes;
	bool disasm_special;
	char numArgs;
	std::string argNames[AXIOM_MAX_ASM_ARGS];		// name of argument
	std::string argSizes[AXIOM_MAX_ASM_ARGS];		// valid values: B<number>; anything else is a template name such as "P"
	std::map<std::string, size_t> argParamSize;		// e.g. "P" -> 0 if 0 is 1st arg to have a size of "P"
	std::list<AxiomCpuEncoding> encodings;
	std::string disasm_second_name;
	bool gotInsnSemantics;
	bool oszSplit, aszSplit, sszSplit, dszSplit;
	std::map<U4, AxiomCpuSemantics> semantics;		// osz+4*asz+16*ssz+64*dsz -> semantics. 0=16bits,1=32bits,2=64bits,3=any bits

	AxiomCpuInsn()
	{
		oszSplit = aszSplit = sszSplit = dszSplit = false;
		name.clear();
		alias = NULL;
		insn_flags = 0;
		valid_modes = 0;
		disasm_special = false;
		for(size_t i = 0; i < AXIOM_MAX_ASM_ARGS; ++i)
		{
			argNames[i].clear();
			argSizes[i].clear();
		}
		argParamSize.clear();
		numArgs = 0;
		disasm_second_name.clear();
		gotInsnSemantics = false;
	}

	bool addArg(std::string name, std::string size)
	{
		if(numArgs >= AXIOM_MAX_ASM_ARGS)
			return false;

		argNames[numArgs] = name;
		argSizes[numArgs] = size;

		int x = getAsmSize(size);
		if(x == 0)
			return false;	// disallow assembly args that have BIT size
		if(x == -1)
		{
			if(argParamSize.find(size) == argParamSize.end())
			{
				argParamSize[size] = numArgs;
			}
		}

		++numArgs;
		return true;
	}
};

class AxiomItemCpuArch :
	public AxiomItemBase
{
	friend class IntelDecoderTableBuilder;

	AxiomItem src;

	std::set<AxiomItem> cpuInsnNames;
	std::set<AxiomItem> allArgTypes;		// these are always identifiers
	std::set<AxiomItem> allArgSizes;		// these are always identifiers, never "BIT" or "B<number>"
	std::set<AxiomItem> allArgValues;		// these are always identifiers; the integer ones are not stored here
	std::map<size_t, AxiomCpuEncoding *> encodings;

	void reset()
	{
		cpuModes.clear();
		coreCpuModesList.clear();
		cpuInsns.clear();
		cpuInsnNames.clear();
		itags.clear();
		etags.clear();
		allArgTypes.clear();
		allArgSizes.clear();
		allArgValues.clear();
	}

public:
	std::map<AxiomItem, size_t> cpuModes;
	std::list<AxiomItem> coreCpuModesList;
	std::map<std::string, AxiomCpuInsn> cpuInsns;	// alias -> insn object
	std::map<AxiomItem, size_t> etags;
	std::map<AxiomItem, size_t> itags;

	size_t getNumEncodings()
	{
		return encodings.size();
	}

	AxiomCpuEncoding *getEncoding(size_t num)
	{
		if(encodings.find(num) == encodings.end())
			return NULL;
		return encodings[num];
	}

	virtual void clear(BaseObjectPool *pool, size_t indexT)
	{
		AxiomItemBase::clear(pool, indexT);
		setType(AXIOM_ITEM_ATOM_CPU_ARCH);
		src.clear();
		reset();
	}

	// Returns true on success; else, we print an error message and return false.
	bool init(std::ostream &oerr, AxiomItem srcList)
	{
		src.clear();
		reset();
		if(srcList->getList() == NULL)
		{
			oerr << "#error: loadcpu expects a list" << std::endl;
			return false;
		}

		for(AxiomItem tmp = srcList; !tmp->getList()->isNil(); tmp = tmp->getList()->getRest())
		{
			AxiomItem item = tmp->getList()->getFirst();
			
			if(item->getList() == NULL)
			{
				oerr << "#error: item in loadcpu list is not a list: ";
				item->write(oerr);
				oerr << std::endl;
				return false;
			}

			if(item->getList()->isNil())
				continue;	// ignore nil items at loadcpu scope
			
			if(!processItem(oerr, item))
				return false;
		}

		src = srcList;
		return true;
	}

	virtual void write(std::ostream &os)
	{
		os << "{cpu architecture}";
	}

	AxiomItemCpuArch()
	{
	}

	virtual ~AxiomItemCpuArch()
	{
	}

private:
	bool syntaxError(std::ostream &oerr, AxiomItem item, int code = -1)
	{
		oerr << "#error: syntax error ";
		if(code != -1)
			oerr << "(code " << code << ") ";
		oerr << "with loadcpu statement: ";
		item->write(oerr);
		oerr << std::endl;
		return false;
	}
	bool processInsn(std::ostream &oerr, AxiomItem insnDef)
	{
		if(insnDef->getList() == NULL || insnDef->getList()->isNil())
			return syntaxError(oerr, insnDef, __LINE__);
		AxiomItem insnName = insnDef->getList()->getFirst();
		if(insnName->getIdentifier() == NULL)
			return syntaxError(oerr, insnDef, __LINE__);
		insnDef = insnDef->getList()->getRest();
		if(insnDef->getList()->isNil())
			return syntaxError(oerr, insnDef, __LINE__);
		AxiomItem insnString = insnDef->getList()->getFirst();
		if(insnString->getString() == NULL)
			return syntaxError(oerr, insnDef, __LINE__);
		insnDef = insnDef->getList()->getRest();
		if(insnDef->getList()->isNil())
			return syntaxError(oerr, insnDef, __LINE__);
		AxiomItem insnArgs = insnDef->getList()->getFirst();
		if(!insnArgs->isList())
			return syntaxError(oerr, insnDef, __LINE__);

		std::string insnStringText = insnString->getString()->getText();
		if(insnStringText.empty())
			return syntaxError(oerr, insnDef, __LINE__);
		if(cpuInsns.find(insnStringText) != cpuInsns.end())
		{
			oerr << "#error: loadcpu: duplicate instruction alias: " << insnStringText << std::endl;
			return false;
		}
		if(cpuInsnNames.find(insnName) != cpuInsnNames.end())
		{
			oerr << "#error: loadcpu: duplicate instruction name: " << insnName->getIdentifier()->getName() << std::endl;
			return false;
		}
		cpuInsnNames.insert(insnName);
		AxiomCpuInsn &insn = cpuInsns[insnStringText];

		size_t argCount = 0;
		while(!insnArgs->getList()->isNil())
		{
			AxiomItem arg = insnArgs->getList()->getFirst();
			if(arg->getList() == NULL)
			{
				oerr << "#error: loadcpu: insn " << insnStringText << ": invalid argument for instruction" << std::endl;
				return false;
			}
			AxiomItem a0 = arg->getList()->getFirst();
			arg = arg->getList()->getRest();
			if(arg->getList()->isNil())
			{
				oerr << "#error: loadcpu: insn " << insnStringText << ": invalid argument for instruction" << std::endl;
				return false;
			}
			AxiomItem a1 = arg->getList()->getFirst();
			arg = arg->getList()->getRest();
			if(!arg->getList()->isNil() || a0->getIdentifier() == NULL || a1->getIdentifier() == NULL)
			{
				oerr << "#error: loadcpu: insn " << insnStringText << ": invalid argument for instruction" << std::endl;
				return false;
			}

			if(!insn.addArg(a1->getIdentifier()->getName(), a0->getIdentifier()->getName()))
			{
				oerr << "#error: loadcpu: insn " << insnStringText << ": unable to add argument number " << argCount << " for instruction" << std::endl;
				return false;
			}

			insnArgs = insnArgs->getList()->getRest();
			++argCount;
		}

		insn.alias = &(cpuInsns.find(insnStringText)->first);
		insn.name = insnName;

		insnDef = insnDef->getList()->getRest();
		while(!insnDef->getList()->isNil())
		{
			if(!processInsnItem(insn, oerr, insnDef->getList()->getFirst(), insnStringText))
			{
				oerr << "#error: loadcpu: insn " << insnStringText << ": invalid item: ";
				insnDef->getList()->getFirst()->write(oerr);
				oerr << std::endl;
				return false;
			}

			insnDef = insnDef->getList()->getRest();
		}

		//std::cerr << " " << insnStringText << std::flush;
		return true;
	}

	std::pair<char, int> getSensitivity(std::string s)
	{
		if(s == "o16" || s == "a16" || s == "s16" || s == "d16")
			return std::pair<char, int>(s[0], 16);
		if(s == "o32" || s == "a32" || s == "s32" || s == "d32")
			return std::pair<char, int>(s[0], 32);
		if(s == "o64" || s == "a64" || s == "s64" || s == "d64")
			return std::pair<char, int>(s[0], 64);
		return std::pair<char, int>('?', 0);
	}

	bool hasPushOrPop(AxiomItem x)
	{
		AxiomItem a = x->getList()->getFirst();
		if(a->getIdentifier() != NULL)
		{
			if(a->getIdentifier()->getName() == "push" ||
				a->getIdentifier()->getName() == "pop"
			)
				return true;
		}
		while(!x->getList()->isNil())
		{
			AxiomItem y = x->getList()->getFirst();

			if(y->isList())
			{
				if(hasPushOrPop(y))
					return true;
			}

			x = x->getList()->getRest();
		}
		return false;
	}

	bool parseSemantics(AxiomCpuInsn &insn, AxiomItem emu, std::ostream &os)
	{
		int osz_count = 1, asz_count = 1, ssz_count = 1, dsz_count = 1;

		// First find out what we're sensitive to.

		if(hasPushOrPop(emu))
		{
			//std::cout << *insn.alias << " : uses push/pop" << std::endl;
			ssz_count = 3;		// push/pop means we're sensitive to ssz
			insn.sszSplit = true;
		}

		AxiomItem x = emu;
		while(!x->getList()->isNil())
		{
			AxiomItem y = x->getList()->getFirst();
			if(!y->isList())
			{
				os << "#error: emu items must be lists" << std::endl;
				return false;
			}
			AxiomItem first = y->getList()->getFirst();
			if(first->getIdentifier() == NULL)
			{
				os << "#error: each list in an emu section must begin with an identifier" << std::endl;
				return false;
			}
			if(first->getIdentifier()->getName() == "split")
			{
				first = y->getList()->getRest();
				if(first->getList()->isNil())
				{
					os << "#error: syntax error: expected (split (modes...) [items...])" << std::endl;
					return false;
				}
				AxiomItem sens = first->getList()->getFirst();
				if(!sens->getList()->isList() || sens->getList()->isNil())
				{
					os << "#error: syntax error: expected (split (modes...) [items...])" << std::endl;
					return false;
				}
				while(!sens->getList()->isNil())
				{
					AxiomItem q = sens->getList()->getFirst();
					if(q->getIdentifier() == NULL)
					{
						os << "#error: invalid split mode: ";
						q->write(os);
						os << std::endl;
						return false;
					}
					std::pair<char, int> p = getSensitivity(q->getIdentifier()->getName());
					if(p.second == 0)
					{
						os << "#error: invalid split mode: " << q->getIdentifier()->getName() << std::endl;
						return false;
					}
					if(p.first == 'o')
					{
						osz_count = 3;
						insn.oszSplit = true;
					}
					if(p.first == 's')
					{
						ssz_count = 3;
						insn.sszSplit = true;
					}
					if(p.first == 'a')
					{
						asz_count = 3;
						insn.aszSplit = true;
					}
					if(p.first == 'd')
					{
						dsz_count = 3;
						insn.dszSplit = true;
					}
					sens = sens->getList()->getRest();
				}
			}

			x = x->getList()->getRest();
		}

		for(int osz = 0; osz < osz_count; ++osz)
		{
			for(int asz = 0; asz < asz_count; ++asz)
			{
				for(int ssz = 0; ssz < ssz_count; ++ssz)
				{
					for(int dsz = 0; dsz < dsz_count; ++dsz)
					{
						if(!processEmuCode(insn, emu, os, osz, asz, ssz, dsz))
							return false;
					}
				}
			}
		}

		return true;
	}

	bool processEmuCode(AxiomCpuInsn &insn, AxiomItem emu, std::ostream &os, int osz, int asz, int ssz, int dsz)
	{
		size_t index = osz + 4 * asz + 16 * ssz + 64 * dsz;
		AxiomCpuSemantics &sem = insn.semantics[index];

		sem.tempNames.clear();
		sem.numAllocTemps = 0;
		AxiomItem x = emu;
		while(!x->getList()->isNil())
		{
			AxiomItem y = x->getList()->getFirst();
			if(!y->isList() || y->getList()->isNil())
				return false;
			AxiomItem z = y->getList()->getFirst();
			if(z->getIdentifier() == NULL)
				return false;
			std::string cmd = z->getIdentifier()->getName();

			if(cmd == "split")
			{
				AxiomItem first = y->getList()->getRest();
				if(first->getList()->isNil())
				{
					os << "#error: syntax error: expected (split (modes...) [items...])" << std::endl;
					return false;
				}
				AxiomItem sens = first->getList()->getFirst();
				if(!sens->getList()->isList() || sens->getList()->isNil())
				{
					os << "#error: syntax error: expected (split (modes...) [items...])" << std::endl;
					return false;
				}
				bool keep = true;
				while(keep && !sens->getList()->isNil())
				{
					AxiomItem q = sens->getList()->getFirst();
					if(q->getIdentifier() == NULL)
					{
						os << "#error: invalid split mode: ";
						q->write(os);
						os << std::endl;
						return false;
					}
					std::pair<char, int> p = getSensitivity(q->getIdentifier()->getName());
					if(p.first == 'o' && (16 << osz) != p.second)
						keep = false;
					else
					if(p.first == 'a' && (16 << asz) != p.second)
						keep = false;
					else
					if(p.first == 's' && (16 << ssz) != p.second)
						keep = false;
					else
					if(p.first == 'd' && (16 << dsz) != p.second)
						keep = false;
					else
					if(p.first != 'o' && p.first != 'a' && p.first != 's' && p.first != 'd')
						return false;

					sens = sens->getList()->getRest();
				}
				if(keep)
				{
					first = first->getList()->getRest();
					while(!first->getList()->isNil())
					{
						if(!processEmuCommand(insn, first->getList()->getFirst(), os, sem))
							return false;
						first = first->getList()->getRest();
					}
				}
			}
			else
			{
				if(!processEmuCommand(insn, y, os, sem))
					return false;
			}

			x = x->getList()->getRest();
		}

		return true;	// success
	}

	bool processEmuCommand(AxiomCpuInsn &insn, AxiomItem item, std::ostream &os, AxiomCpuSemantics &sem)
	{
		AxiomCpuSemanticsNode *node = processEmuNode(insn, item, os, sem, false, false);
		if(node == NULL)
		{
			os << "#error: here: ";
			item->write(os);
			os << std::endl;
			return false;
		}
		if(!AxiomCpuSemantics::isCommand(node->opcode) || node->operands.empty())
		{
			os << "#error: invalid command '" << node->opcode << "'" << std::endl;
			return false;
		}
		sem.commands.push_back(node);
		return true;
	}

	AxiomCpuSemanticsNode *processEmuNode(AxiomCpuInsn &insn, AxiomItem item, std::ostream &os, AxiomCpuSemantics &sem, bool in_if, bool in_times)
	{
		if(!item->isList())
		{
			// Create a terminal node.
			if(item->getInteger() != NULL)
			{
				// This is a B8 number.
				sem.nodes.push_back(AxiomCpuSemanticsNode());
				AxiomCpuSemanticsNode *node = &sem.nodes.back();
				node->opcode = "$num";
				node->tag = 8;
				node->value[0] = item->getInteger()->getScalarValue();
				node->value[1] = 0;
				return node;
			}
			if(item->getIdentifier() == NULL)
			{
				os << "#error: unsupported operand: ";
				item->write(os);
				os << std::endl;
				return NULL;
			}
			std::string s = item->getIdentifier()->getName();
			if(getAsmSize(s) != -1)
			{
				os << "#error: unexpected size: " << s << std::endl;
				return NULL;
			}
			for(size_t i = 0; i < (size_t)(insn.numArgs); ++i)
			{
				if(s == insn.argNames[i])
				{
					sem.nodes.push_back(AxiomCpuSemanticsNode());
					AxiomCpuSemanticsNode *node = &sem.nodes.back();
					node->opcode = "$arg";
					node->tag = i;
					node->value[0] = 0;
					node->value[1] = 0;
					return node;
				}
				if(s == insn.argSizes[i])
				{
					sem.nodes.push_back(AxiomCpuSemanticsNode());
					AxiomCpuSemanticsNode *node = &sem.nodes.back();
					node->opcode = "$argsize";
					node->tag = i;
					node->value[0] = 0;
					node->value[1] = 0;
					return node;
				}
			}
			if(sem.tempNames.find(s) != sem.tempNames.end())
			{
					sem.nodes.push_back(AxiomCpuSemanticsNode());
					AxiomCpuSemanticsNode *node = &sem.nodes.back();
					node->opcode = "$tmp";
					node->tag = 0;
					node->value[0] = sem.tempNames[s];
					node->value[1] = 0;
					return node;
			}
			// Other identifier.
			sem.nodes.push_back(AxiomCpuSemanticsNode());
			AxiomCpuSemanticsNode *node = &sem.nodes.back();
			node->opcode = s;
			node->tag = 0;
			node->value[0] = 0;
			node->value[1] = 0;
			return node;
		}

		if(item->getList()->isNil())
			return NULL;

		// First item in 'item' list should be a command.
		AxiomItem first = item->getList()->getFirst();
		if(first->getIdentifier() == NULL)
		{
			os << "#error: unexpected item: ";
			first->write(os);
			os << std::endl;
			return NULL;
		}
		std::string cmd = first->getIdentifier()->getName();
		first = item;
		/*if(!AxiomCpuSemantics::isCommand(cmd))
		{
			os << "#error: unrecognized emu command: " << cmd << std::endl;
			return false;
		}*/
		if(cmd == "split")
		{
			os << "#error: split allowed only at global emu scope" << std::endl;
			return NULL;
		}
		if(cmd == "alloc")
		{
			if(in_if || in_times)
			{
				os << "#error: attempt to use 'alloc' inside an 'if' or 'times' block" << std::endl;
				return NULL;
			}
		}
		else
		if(cmd == "if")
			in_if = true;
		else
		if(cmd == "times")
		{
			if(in_times)
			{
				os << "#error: attempt to nest 'times' command" << std::endl;
				return NULL;
			}
			in_times = true;
		}

		// Handle special functions/commands first.
		if(cmd == "_zx" || cmd == "_sx")
		{
			first = first->getList()->getRest();	// skip function name
			if(first->getList()->isNil())
			{
				os << "#error: invalid use of function: " << cmd << std::endl;
				return NULL;
			}
			AxiomItem size = first->getList()->getFirst();
			first = first->getList()->getRest();	// skip size
			if(first->getList()->isNil())
			{
				os << "#error: invalid use of function: " << cmd << std::endl;
				return NULL;
			}
			AxiomItem src = first->getList()->getFirst();
			first = first->getList()->getRest();	// skip src
			if(!first->getList()->isNil() || size->getIdentifier() == NULL)
			{
				os << "#error: invalid use of function: " << cmd << std::endl;
				return NULL;
			}

			std::string t = size->getIdentifier()->getName();
			int x = getAsmSize(t);
			if(x == -1)
			{
				// Check to see if we're _zx/_sx'ing to an arg size.
				x = 128;
				for(size_t i = 0; i < (size_t)(insn.numArgs); ++i)
				{
					if(insn.argSizes[i] == t)
					{
						x = -1 - (int)(i);
						break;
					}
				}
				if(x == 128)
				{
					os << "#error: unable to " << cmd << " to this size: " << t << std::endl;
					return NULL;
				}
			}

			sem.nodes.push_back(AxiomCpuSemanticsNode());
			AxiomCpuSemanticsNode *node = &sem.nodes.back();
			node->opcode = cmd;
			node->tag = x;
			node->value[0] = 0;
			node->value[1] = 0;
			AxiomCpuSemanticsNode *n = processEmuNode(insn, src, os, sem, in_if, in_times);
			if(n == NULL)
				return NULL;
			node->operands.push_back(n);
			return node;
		}

		int sz = getAsmSize(cmd);
		if(sz != -1)
		{
			if(sz == 0 || sz > 16)
			{
				os << "#error: invalid size of literal number: " << cmd << std::endl;
				return NULL;
			}
			first = first->getList()->getRest();
			if(first->getList()->isNil())
			{
				os << "#error: invalid literal number" << std::endl;
				return NULL;
			}
			AxiomItem a = first->getList()->getFirst();
			first = first->getList()->getRest();
			AxiomItem b;
			if(first->getList()->isNil())
				b = first;
			else
			{
				b = first->getList()->getFirst();
				first = first->getList()->getRest();
				if(!first->getList()->isNil())
				{
					os << "#error: invalid literal number" << std::endl;
					return NULL;
				}
			}
			U8 val[2];
			val[0] = 0;
			val[1] = 0;

			if(a->getInteger() == NULL)
			{
				os << "#error: invalid literal number" << std::endl;
				return NULL;
			}
			val[0] = a->getInteger()->getScalarValue();
			if(b->isList() && sz > 8)
			{
				os << "#error: invalid literal number" << std::endl;
				return NULL;
			}
			if(b->getInteger() != NULL)
			{
				val[1] = b->getInteger()->getScalarValue();
			}

			sem.nodes.push_back(AxiomCpuSemanticsNode());
			AxiomCpuSemanticsNode *node = &sem.nodes.back();
			node->opcode = "$num";
			node->tag = sz;
			node->value[0] = val[0];
			node->value[1] = val[1];
			return node;
		}

		// Regular function/command invokation. Make sure we have at least one argument.
		first = first->getList()->getRest();
		if(first->getList()->isNil())
		{
			os << "#error: command/function must have at least one argument" << std::endl;
			return NULL;
		}

		sem.nodes.push_back(AxiomCpuSemanticsNode());
		AxiomCpuSemanticsNode *node = &sem.nodes.back();
		node->opcode = cmd;
		node->tag = 0;
		node->value[0] = 0;
		node->value[1] = 0;

		if(cmd == "alloc")
		{
			AxiomItem size = first->getList()->getFirst();
			first = first->getList()->getRest();
			if(first->getList()->isNil())
			{
				os << "#error: invalid alloc syntax [1]" << std::endl;
				return NULL;
			}
			AxiomItem name = first->getList()->getFirst();
			first = first->getList()->getRest();
			if(first->getList()->isNil())
			{
				os << "#error: alloc must provide initial value" << std::endl;
				return NULL;
			}
			if(name->getIdentifier() == NULL || size->getIdentifier() == NULL)
			{
				os << "#error: invalid alloc syntax [2]" << std::endl;
				return NULL;
			}
			std::string t = size->getIdentifier()->getName();
			int tsz = getAsmSize(t);
			if(tsz < 0 || tsz >= 127)
			{
				tsz = 128;
				for(size_t i = 0; i < (size_t)(insn.numArgs); ++i)
				{
					if(insn.argSizes[i] == t)
					{
						tsz = -1 - (int)(i);
						break;
					}
				}
				if(tsz == 128)
				{
					os << "#error: invalid alloc syntax [3]" << std::endl;
					return NULL;
				}
			}
			node->tag = tsz;
			sem.tempNames[name->getIdentifier()->getName()] = sem.numAllocTemps;
			++sem.numAllocTemps;
		}

		while(!first->getList()->isNil())
		{
			AxiomCpuSemanticsNode *n = processEmuNode(insn, first->getList()->getFirst(), os, sem, in_if, in_times);
			if(n == NULL)
				return NULL;
			node->operands.push_back(n);
			first = first->getList()->getRest();
		}

		return node;
	}

	bool processInsnItem(AxiomCpuInsn &insn, std::ostream &oerr, AxiomItem item, std::string insnAlias)
	{
		if(item->getList() == NULL || item->getList()->isNil())
			return false;
		AxiomItem first = item->getList()->getFirst();
		if(first->getIdentifier() == NULL)
			return syntaxError(oerr, item);
		std::string sfirst = first->getIdentifier()->getName();

		if(sfirst == "valid-modes")
		{
			AxiomItem tmp = item->getList()->getRest();
			while(!tmp->getList()->isNil())
			{
				AxiomItem x = tmp->getList()->getFirst();
				if(x->getIdentifier() == NULL)
					return false;

#if 1
				if(x->getIdentifier()->getName() == "TODO")
				{
					oerr << "#warning: need valid-modes for instruction: " << insnAlias << std::endl;
					tmp = tmp->getList()->getRest();
					continue;
				}
#endif

				if(cpuModes.find(x) == cpuModes.end())
				{
					oerr << "#error: can't find valid cpu mode: " << x->getIdentifier()->getName() << std::endl;
					return false;
				}
				insn.valid_modes = cpuModes[x];

				tmp = tmp->getList()->getRest();
			}
		}
		else
		if(sfirst == "flags")
		{
			AxiomItem tmp = item->getList()->getRest();
			size_t tags = 0;
			while(!tmp->getList()->isNil())
			{
				AxiomItem x = tmp->getList()->getFirst();
				if(x->getIdentifier() == NULL)
					return false;

				if(itags.find(x) == itags.end())
				{
					if(itags.size() >= 30)
					{
						oerr << "#error: too many itags, trying to add: " << x->getIdentifier()->getString() << std::endl;
					}
					size_t n = 1 << itags.size();
					itags[x] = n;
				}
				tags |= itags[x];
				tmp = tmp->getList()->getRest();
			}
			insn.insn_flags = tags;
		}
		else
		if(sfirst == "emu")
		{
			AxiomItem tmp = item->getList()->getRest();		// skip 'emu'
			if(tmp->getList()->isNil())
				return false;
			AxiomItem emu_type = tmp->getList()->getFirst();
			if(emu_type->getIdentifier() == NULL)
			{
				oerr << "#error: expected (emu basic/intermediate/advanced [items...])" << std::endl;
				return false;
			}
			insn.gotInsnSemantics = true;
			tmp = tmp->getList()->getRest();
			if(!tmp->getList()->isNil())
			{
				if(!parseSemantics(insn, tmp, oerr))
					return false;
			}
		}
		else
		if(sfirst == "disasm")
		{
			AxiomItem tmp = item->getList()->getRest();
			if(tmp->getList()->isNil())
				return false;
			AxiomItem a = tmp->getList()->getFirst();
			tmp = tmp->getList()->getRest();
			if(!tmp->getList()->isNil())
				return false;
			if(a->getString() != NULL)
				insn.disasm_second_name = a->getString()->getText();
			else
			{
				insn.disasm_second_name.clear();
				if(a->getIdentifier() == NULL)
					return false;
				if(a->getIdentifier()->getName() != "special")
					return false;
			}
			insn.disasm_special = true;
		}
		else
		if(sfirst == "code")
		{
			AxiomItem tmp = item->getList()->getRest();		// skip code
			if(tmp->getList()->isNil())
				return false;
			AxiomItem etagsT = tmp->getList()->getFirst();
			if(!etagsT->isList())
				return false;
			tmp = tmp->getList()->getRest();
			if(tmp->getList()->isNil())
				return false;
			AxiomItem opcodes = tmp->getList()->getFirst();
			if(opcodes->getString() == NULL)
				return false;
			tmp = tmp->getList()->getRest();
			if(tmp->getList()->isNil())
				return false;
			AxiomItem args = tmp->getList()->getFirst();
			tmp = tmp->getList()->getRest();
			if(!tmp->getList()->isNil() || args->getList() == NULL)
				return false;
			
			insn.encodings.push_back(AxiomCpuEncoding());
			AxiomCpuEncoding &enc = insn.encodings.back();
			enc.insn = &insn;
			enc.opcodes = opcodes->getString()->getText();

			enc.encoding_num = encodings.size();
			encodings[enc.encoding_num] = &enc;

			enc.etags = 0;
			while(!etagsT->getList()->isNil())
			{
				if(etagsT->getList()->getFirst()->getIdentifier() == NULL)
				{
					oerr << "#error: invalid encoding tag: ";
					etagsT->getList()->getFirst()->write(oerr);
					oerr << std::endl;
				}
				AxiomItem tag = etagsT->getList()->getFirst();

				if(etags.find(tag) == etags.end())
				{
					if(etags.size() >= 30)
					{
						oerr << "#error: too many encoding tags, trying to add: " << tag->getIdentifier()->getName() << std::endl;
						return false;
					}
					size_t n = 1 << etags.size();
					etags[tag] = n;
				}
				enc.etags |= etags[tag];

				etagsT = etagsT->getList()->getRest();
			}

			size_t numArgs = 0;
			while(!args->getList()->isNil())
			{
				AxiomItem a = args->getList()->getFirst();

				if(!a->isList() || a->getList()->isNil())
				{
					oerr << "#error: [1] invalid argument number " << numArgs << " for encoding" << std::endl;
					return false;
				}

				AxiomItem atype = a->getList()->getFirst();
				if(atype->getIdentifier() == NULL)
				{
					oerr << "#error: [4] invalid argument number " << numArgs << " for encoding" << std::endl;
					return false;
				}

				a = a->getList()->getRest();
				if(a->getList()->isNil())
				{
					oerr << "#error: [5] invalid argument number " << numArgs << " for encoding" << std::endl;
					return false;
				}

				AxiomItem asize = a->getList()->getFirst();
				if(asize->getIdentifier() == NULL)
				{
					oerr << "#error: [2] invalid argument number " << numArgs << " for encoding" << std::endl;
					return false;
				}

				a = a->getList()->getRest();
				AxiomItem avalue_opt;		// argument value, NULL means default value
				if(!a->getList()->isNil())
				{
					avalue_opt = a->getList()->getFirst();
					a = a->getList()->getRest();
					if(!a->getList()->isNil())
					{
						oerr << "#error: [3] invalid argument number " << numArgs << " for encoding" << std::endl;
						return false;
					}

					if(avalue_opt->getIdentifier() == NULL)
					{
						if(avalue_opt->getInteger() == NULL || avalue_opt->getInteger()->getSizeBytes() != 8 ||
							avalue_opt->getInteger()->getScalarValue() > 127
						)
						{
							oerr << "#error: argument number " << numArgs << " must be an integer or QWORD identifier with value 0..127 inclusive" << std::endl;
							return false;
						}
					}
				}

				if(numArgs >= AXIOM_MAX_ASM_ARGS)
				{
					oerr << "#error: too many arguments, we only support " << AXIOM_MAX_ASM_ARGS << std::endl;
					return false;
				}

				enc.arg_types[numArgs] = atype;
				enc.arg_sizes[numArgs] = asize;
				enc.arg_values[numArgs].clear();
				if(!avalue_opt.isNull())
					enc.arg_values[numArgs] = avalue_opt;

				allArgTypes.insert(atype);
				if(getAsmSize(asize->getIdentifier()->getName()) == -1)
				{
					allArgSizes.insert(asize);
				}
				if(!avalue_opt.isNull())
				{
					if(avalue_opt->getIdentifier() != NULL)
						allArgValues.insert(avalue_opt);
				}

				++numArgs;
				args = args->getList()->getRest();
			}

			if(numArgs != insn.numArgs)
			{
				oerr << "#error: mismatch in argument count for encoding" << std::endl;
				return false;
			}
		}
		else
			return false;

		return true;
	}

	bool processItem(std::ostream &oerr, AxiomItem item)
	{
		AxiomItem first = item->getList()->getFirst();
		if(first->getIdentifier() == NULL)
			return syntaxError(oerr, item);
		std::string sfirst = first->getIdentifier()->getName();
		
		if(sfirst == "insn")
		{
			return processInsn(oerr, item->getList()->getRest());
		}
		else
		if(sfirst == "set-cpu-modes")
		{
			AxiomItem x = item->getList()->getRest();
			while(!x->getList()->isNil())
			{
				AxiomItem y = x->getList()->getFirst();
				if(y->getIdentifier() == NULL || cpuModes.find(y) != cpuModes.end() || cpuModes.size() >= 30)
					return syntaxError(oerr, item);
				size_t tmp = 1 << cpuModes.size();
				cpuModes[y] = tmp;
				coreCpuModesList.push_back(y);
				x = x->getList()->getRest();
			}
		}
		else
		if(sfirst == "define-mode-set")
		{
			AxiomItem x = item->getList()->getRest();
			if(x->getList()->isNil())
				return syntaxError(oerr, item, __LINE__);
			AxiomItem name = x->getList()->getFirst();
			if(name->getIdentifier() == NULL)
				return syntaxError(oerr, item, __LINE__);

			x = x->getList()->getRest();
			size_t value = 0;

			if(x->getList()->isNil())
				return syntaxError(oerr, item);
			AxiomItem z = x->getList()->getRest();
			if(!z->getList()->isNil())
				return syntaxError(oerr, item, __LINE__);
			x = x->getList()->getFirst();
			if(!x->isList())
				return syntaxError(oerr, item, __LINE__);

			while(!x->getList()->isNil())
			{
				AxiomItem y = x->getList()->getFirst();
				if(y->getIdentifier() == NULL)
					return syntaxError(oerr, item, __LINE__);

				if(cpuModes.find(y) == cpuModes.end())
				{
					oerr << "#error: loadcpu item: can't find cpu mode: " << y->getIdentifier()->getName() << std::endl;
					return false;
				}
				value |= cpuModes[y];

				x = x->getList()->getRest();
			}

			if(value == 0 || cpuModes.find(name) != cpuModes.end())
				return syntaxError(oerr, item, __LINE__);
			cpuModes[name] = value;
		}
		else
		{
			oerr << "#error: unknown loadcpu command '" << sfirst << "'" << std::endl;
			return false;
		}

		return true;
	}
};

#endif	// l_crudasm__axiom_cpu_arch_h__included
