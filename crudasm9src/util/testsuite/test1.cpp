// test1.cpp - Copyright (C) 2012 Willow Schlanger. All rights reserved.
// g++ -o test1 test1.cpp ../../x86core/decoder_internal.c ../../x86core/ixdecoder.c ../../x86core/ixdisasm.c

#include "../../x86core/ixdisasm.h"
#include <stddef.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>

int main(int argc, char **argv)
{
	using namespace std;

	for(int dsz = 0; dsz <= 2; ++dsz)
	{
		std::cout << (16 << dsz) << "-bit mode:" << std::flush;
		const char *fnames[3] = {"test16.bin", "test32.bin", "test64.bin"};
		FILE *fo = fopen(fnames[dsz], "wb");
		
		const char *prev = NULL;
		for(size_t enc_idx = 0; crudasm_intel_encoding_table[enc_idx].insn != crudasm_intel_insncount; ++enc_idx)
		{
			crudasm_intel_encoding_t &enc = crudasm_intel_encoding_table[enc_idx];
			U4 insn_num = enc.insn;
			crudasm_intel_insn_t &insn = crudasm_intel_insns[insn_num];
			const char *alias = insn.alias;
			if(alias != prev)
			{
				std::cout << " " << alias << std::flush;
				prev = alias;
			}
			
			if((enc.etags & (crudasm_intel_etag_asm_skip)) != 0)
			{
				continue;
			}
			
			if(dsz == 2 /*64bit mode*/ && (enc.etags & (crudasm_intel_etag_no64)) != 0)
				continue;
			
			int replock_count = 1;
			if((insn.itags & crudasm_intel_itag_repable) != 0 || (insn.itags & crudasm_intel_itag_repcond))
				replock_count = 3;	// none, f2, f3
			else
			if((insn.itags & crudasm_intel_itag_lockable) != 0 || (insn.itags & crudasm_intel_itag_lock_always) != 0)
				replock_count = 2;	// none, lock (f0)
			
			int fwait_count = 1;
			if((insn.itags & crudasm_intel_itag_fwaitable) != 0)
				fwait_count = 2;	// none, wait (9b)
			
			if((enc.etags & crudasm_intel_etag_like_arpl) != 0)
			{
				if(dsz == 2)
					continue;	// no arpl in 64bit mode
			}
			else
			if((enc.etags & crudasm_intel_etag_like_movsxd) != 0)
			{
				if(dsz != 2)
					continue;	// movsxd exists only in 64bit mode
			}
			
			for(int replock_index = 0; replock_index < replock_count; ++replock_index)
			{
				for(int fwait_index = 0; fwait_index < fwait_count; ++fwait_index)
				{
					U1 buf[32] = {0};
					size_t index = 0;

					// This must be 1st, it's technically a SEPARATE instruction.
					if(fwait_count == 2 && fwait_index != 0)
					{
						buf[index++] = 0x9b;
					}
					
					if((enc.etags & crudasm_intel_etag_like_movsxd) != 0)
					{
						buf[index++] = 0x48;	// rex.w for movsxd
					}
					
					bool did66 = false;
					if(enc.op66 == 2 || (dsz == 0 && (enc.etags & crudasm_intel_etag_asm_66_if_o16) != 0))
					{
						buf[index++] = 0x66;
						did66 = true;
					}

					if(replock_count == 3 && replock_index != 0)
					{
						buf[index++] = (replock_index == 1) ? 0xf2 : 0xf3;
					}
					else
					if(replock_count == 2 && replock_index != 0)
					{
						buf[index++] = 0xf0;
					}

					if(enc.fx == 2)
					{
						buf[index++] = 0xf2;
					}
					else
					if(enc.fx == 3)
					{
						buf[index++] = 0xf3;
					}
					
					U4 opcode1 = enc.opcode1;
					
					if(opcode1 >= 0x200)
					{
						buf[index++] = 0x0f;
						buf[index++] = 0x0f;
					}
					else
					if(opcode1 >= 0x100)
					{
						buf[index++] = 0x0f;
						buf[index++] = (opcode1 & 0xff);
					}
					else
					{
						buf[index++] = (opcode1 & 0xff);
					}
					
					if(enc.opcode2 != 0x100)
					{
						buf[index++] = (enc.opcode2 & 0xff);
					}
					
					if(enc.regop != 0x0f)
					{
						U1 modrm0 = 0;
						if(enc.regop <= 7)
						{
							modrm0 = (enc.regop << 3);
						}
						if(enc.mod <= 3)
							modrm0 |= (enc.mod << 6);
						if(enc.rm <= 7)
							modrm0 |= enc.rm;
						buf[index++] = modrm0;
					}
					
					if(opcode1 >= 0x200)
					{
						// 3DNow! instruction
						buf[index++] = (opcode1 & 0xff);
					}
					
					int osz = (dsz <= 1) ? dsz : 1;
					if(dsz == 2 && (enc.etags & crudasm_intel_etag_is64) != 0)
						osz = 2;
					
					bool lockable = false;
					if(enc.argtype[0] != crudasm_intel_argtype_void)
					{
						if(crudasm_intel_argtype__is_mem(enc.argtype[0]))
							lockable = true;
						if(crudasm_intel_argtype__is_reg(enc.argtype[0]))
						{
							if(enc.argvalue[0] == crudasm_intel_argvalue_reg_or_mem)
								lockable = true;
						}
					}
					if(replock_count == 2 && replock_index != 0 && !lockable)
						continue;	// only use lock if permitted
					
					int disp_size = 0, imm_size = 0;
					for(size_t a = 0; a < CRUDASM_MAX_ASM_ARGS; ++a)
					{
						if(enc.argtype[a] == crudasm_intel_argtype_void)
							break;
						if(crudasm_intel_argtype__is_imm(enc.argtype[a]) && enc.argtype[a] != crudasm_intel_argtype_imm_implict)
						{
							if(enc.argsize[a] >= 1)
								imm_size += enc.argsize[a];
							else
							if(dsz == 2 && enc.argsize[a] == crudasm_intel_argsize_size_osz_64in64 &&
								!(enc.etags & crudasm_intel_etag_sx_byte) &&
								!(enc.etags & crudasm_intel_etag_imm64_sx32)
							)
							{
								// NOTE: This never happens, so it's a bogus case.
								imm_size += 8;
							}
							else
							if(enc.argsize[a] == crudasm_intel_argsize_size_osz || (enc.etags & crudasm_intel_argsize_size_osz_64in64) != 0)
							{
								if((enc.etags & crudasm_intel_etag_sx_byte) != 0)
									imm_size += 1;
								else
								if(osz == 2 && (enc.etags & crudasm_intel_etag_imm64_disp) != 0)
									imm_size += 8;
								else
								if(osz == 2 && (enc.etags & crudasm_intel_etag_imm64_sx32) != 0)
									imm_size += 4;
								else
									imm_size += (2 << osz);
							}
							else
							{
								std::cout << "\n\nERROR - Unsupported argument size " << (int)enc.argsize[a] << " for immediate" << std::endl;
								fclose(fo);
								return 1;
							}
						}
						else
						if(enc.argtype[a] == crudasm_intel_argtype_mem_fulldisp)
						{
							disp_size += (2 << dsz);	// address-size has same size as default size
						}
					}
					
					for(size_t i = 0; i < disp_size; ++i)
						buf[index++] = 0x22;
					for(size_t i = 0; i < imm_size; ++i)
						buf[index++] = 0x11;
					
					fwrite(buf, index, 1, fo);
				}
			}
		}
		fclose(fo);
		std::cout << "\n" << std::endl;
	}

	return 0;
}
