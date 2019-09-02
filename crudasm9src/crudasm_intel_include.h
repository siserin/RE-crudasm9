// crudasm_intel_include.h - Copyright (C) 2012 Willow Schlanger. All rights reserved.

#ifndef l_crudasm_intel_include_h__included
#define l_crudasm_intel_include_h__included

typedef unsigned long long U8;
typedef unsigned int U4;
typedef unsigned short U2;
typedef unsigned char U1;

typedef signed long long S8;
typedef signed int S4;
typedef signed short S2;
typedef signed char S1;

#define TULL(x) (x##ULL)
#define TSLL(x) (x##LL)

#include "generated/out_intel_encoding_info.h"

enum
{
	crudasm_intel_argtype_void = 0,	// 'void' must be 0
	crudasm_intel_argtype_imm,	// (organizer)
		crudasm_intel_argtype_imm_implict = crudasm_intel_argtype_imm,
		crudasm_intel_argtype_imm_1st,
		crudasm_intel_argtype_imm_2nd,
		crudasm_intel_argtype_imm_both,
	crudasm_intel_argtype_imm__end = crudasm_intel_argtype_imm_both,
	crudasm_intel_argtype_mem,	// (organizer)
		crudasm_intel_argtype_mem_fulldisp = crudasm_intel_argtype_mem,
		crudasm_intel_argtype_mem_ea,
			crudasm_intel_argtype_mem_ea_lim,	// for LGDT/SGDT/LIDT/SIDT and related instructions
			crudasm_intel_argtype_mem_ea_dbl,	// for BOUND and cousins
			crudasm_intel_argtype_mem_ea_seg,	// for LES/LSS/LDS/LFS/LGS and cousins
			crudasm_intel_argtype_mem_ea_eal,	// size = asz; for lea and cousins
			crudasm_intel_argtype_mem_ea_eai,	// for INVLPG and cousins
			crudasm_intel_argtype_mem_ea_fxs,	// for fxsave and cousins
		crudasm_intel_argtype_mem_ea__end = crudasm_intel_argtype_mem_ea_fxs,
		crudasm_intel_argtype_mem_implict,	// (organizer)
			crudasm_intel_argtype_mem_implict_sts = crudasm_intel_argtype_mem_implict,
			crudasm_intel_argtype_mem_implict_std,
			crudasm_intel_argtype_mem_implict_xls,
		crudasm_intel_argtype_mem_implict__end = crudasm_intel_argtype_mem_implict_xls,
	crudasm_intel_argtype_mem__end = crudasm_intel_argtype_mem_implict__end,
	crudasm_intel_argtype_reg,	// (organizer)
		crudasm_intel_argtype_reg_gr = crudasm_intel_argtype_reg,
		crudasm_intel_argtype_reg_xmm,
		crudasm_intel_argtype_reg_mmx,
		crudasm_intel_argtype_reg_sr,
		crudasm_intel_argtype_reg_dr,
		crudasm_intel_argtype_reg_cr,
		crudasm_intel_argtype_reg_st,
	crudasm_intel_argtype_reg__end = crudasm_intel_argtype_reg_st
};

enum
{
	crudasm_intel_argsize_void = 0,
	crudasm_intel_argsize_B1 = 1,
	crudasm_intel_argsize_B2 = 2,
	crudasm_intel_argsize_B4 = 4,
	crudasm_intel_argsize_B8 = 8,
	crudasm_intel_argsize_B16 = 16,
	crudasm_intel_argsize_B32 = 32,
	crudasm_intel_argsize_B10 = 10,
	crudasm_intel_argsize_size_osz = -1,			// operand size
	crudasm_intel_argsize_size_asz = -2,			// address size
	crudasm_intel_argsize_size_osz_ptr = -3,		// 64 bits in 64 bit mode regardless of rex; else 32 bits
	crudasm_intel_argsize_size_osz_max32 = -4,		// 16 or 32 bits only (ignore rex.w for this argument)
	crudasm_intel_argsize_size_osz_min32 = -5,		// 32 or 64 bits only (if o16, use 32 bits)
	crudasm_intel_argsize_size_osz_min64 = -6,		// 64 bits (128 bits if rex.w is used)
	crudasm_intel_argsize_size_osz_seg = -7,		// 16 bits unless rex.w is used, then 64 bits (mov dest,sreg [intel manual])
	crudasm_intel_argsize_size_osz_64in64 = -8		// osz, but ALWAYS 64 bits in 64 bit mode
};

int crudasm_intel_argtype__is_imm(U1 x);
int crudasm_intel_argtype__is_reg(U1 x);
int crudasm_intel_argtype__is_mem(U1 x);
int crudasm_intel_argsize__osz_sensitive(S1 x);

enum
{
	crudasm_intel_argvalue_default = 0x80,
	crudasm_intel_argvalue_reg_or_mem = 0x81
};

#endif	// l_crudasm_intel_include_h__included
