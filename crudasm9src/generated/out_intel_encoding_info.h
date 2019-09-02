// out_intel_encoding_info.h  (note: this file was automatically generated -- do not edit!)
// Copyright (C) 2011,2012 Willow Schlanger. All rights reserved.

enum { CRUDASM_MAX_ASM_ARGS = 4 };

struct crudasm_intel_encoding_t {
	U4 insn : 30;	// instruction number (index)
	U4 op66 : 2;	// 0=default, 1=no66, 2=66
	// opcode1 : 100..1ff = 0f <low 8 bits>
	U4 opcode1 : 10;	// 0..2ff (200..2ff = 3DNow!, opcode 2 is the 3DNow! imm8 opcode after modr/m)
	U4 opcode2 : 9;	// 0..ff, 100 for none
	U4 mod : 3;	// 0..3, 4={0,1,2} (i.e. /mdm), 7=none
	U4 regop : 4;	// 0..7, 8=/r, f=none
	U4 rm : 4;	// 0..7, f=none
	U4 fx : 2;	// 0=nofx, 1=default, 2=f2, 3=f3
	U1 argtype[CRUDASM_MAX_ASM_ARGS];	// 0 for void
	S1 argsize[CRUDASM_MAX_ASM_ARGS];	// 1..127 bytes; 0 for void. If <0, see enumeration
	U1 argvalue[CRUDASM_MAX_ASM_ARGS];	// usually 0..127; default value is 0x80, 0x81 means reg_or_mem
	U4 etags;	// encoding tags
};

#ifdef __cplusplus
extern "C" {
#endif

extern struct crudasm_intel_encoding_t crudasm_intel_encoding_table[];  /* see out_intel_encoding_table.h */

extern U4 crudasm_intel_decoder_table[];  /* see out_intel_decoder_table.h */

#ifdef __cplusplus
}
#endif

enum {
	crudasm_intel_insn__aad,
	crudasm_intel_insn__aam,
	crudasm_intel_insn__bt_mem_reg,
	crudasm_intel_insn__bt_other,
	crudasm_intel_insn__callfd,
	crudasm_intel_insn__callfi,
	crudasm_intel_insn__calli,
	crudasm_intel_insn__cmps,
	crudasm_intel_insn__cmpxchgxb,
	crudasm_intel_insn__cmul2,
	crudasm_intel_insn__cmul3,
	crudasm_intel_insn__divb,
	crudasm_intel_insn__fmul1,
	crudasm_intel_insn__fmul2,
	crudasm_intel_insn__fstcw,
	crudasm_intel_insn__fxch,
	crudasm_intel_insn__fxrstor,
	crudasm_intel_insn__fxsave,
	crudasm_intel_insn__idivb,
	crudasm_intel_insn__imulb,
	crudasm_intel_insn__ins,
	crudasm_intel_insn__int3,
	crudasm_intel_insn__iret,
	crudasm_intel_insn__jmpfd,
	crudasm_intel_insn__jmpfi,
	crudasm_intel_insn__jmpi,
	crudasm_intel_insn__jrcxz,
	crudasm_intel_insn__lods,
	crudasm_intel_insn__loop,
	crudasm_intel_insn__loopnz,
	crudasm_intel_insn__loopz,
	crudasm_intel_insn__movcr,
	crudasm_intel_insn__movdr,
	crudasm_intel_insn__movs,
	crudasm_intel_insn__movsd2,
	crudasm_intel_insn__movsrv,
	crudasm_intel_insn__movvsr,
	crudasm_intel_insn__mulb,
	crudasm_intel_insn__nopmb,
	crudasm_intel_insn__outs,
	crudasm_intel_insn__pop,
	crudasm_intel_insn__popa,
	crudasm_intel_insn__popf,
	crudasm_intel_insn__popsr,
	crudasm_intel_insn__push,
	crudasm_intel_insn__pusha,
	crudasm_intel_insn__pushf,
	crudasm_intel_insn__pushsr,
	crudasm_intel_insn__ret,
	crudasm_intel_insn__retf,
	crudasm_intel_insn__retfnum,
	crudasm_intel_insn__retnum,
	crudasm_intel_insn__sal,
	crudasm_intel_insn__scas,
	crudasm_intel_insn__stos,
	crudasm_intel_insn__sxacc,
	crudasm_intel_insn__sxdax,
	crudasm_intel_insn__test,
	crudasm_intel_insn__uint1,
	crudasm_intel_insn__usalc,
	crudasm_intel_insn__xchg,
	crudasm_intel_insn__xlat,
	crudasm_intel_insn_aaa,
	crudasm_intel_insn_aas,
	crudasm_intel_insn_adc,
	crudasm_intel_insn_add,
	crudasm_intel_insn_and,
	crudasm_intel_insn_arpl,
	crudasm_intel_insn_bound,
	crudasm_intel_insn_bsf,
	crudasm_intel_insn_bsr,
	crudasm_intel_insn_bswap,
	crudasm_intel_insn_btc,
	crudasm_intel_insn_btr,
	crudasm_intel_insn_bts,
	crudasm_intel_insn_call,
	crudasm_intel_insn_clc,
	crudasm_intel_insn_cld,
	crudasm_intel_insn_cli,
	crudasm_intel_insn_clts,
	crudasm_intel_insn_cmc,
	crudasm_intel_insn_cmova,
	crudasm_intel_insn_cmovbe,
	crudasm_intel_insn_cmovc,
	crudasm_intel_insn_cmovg,
	crudasm_intel_insn_cmovge,
	crudasm_intel_insn_cmovl,
	crudasm_intel_insn_cmovle,
	crudasm_intel_insn_cmovnc,
	crudasm_intel_insn_cmovno,
	crudasm_intel_insn_cmovnp,
	crudasm_intel_insn_cmovns,
	crudasm_intel_insn_cmovnz,
	crudasm_intel_insn_cmovo,
	crudasm_intel_insn_cmovp,
	crudasm_intel_insn_cmovs,
	crudasm_intel_insn_cmovz,
	crudasm_intel_insn_cmp,
	crudasm_intel_insn_cmpxchg,
	crudasm_intel_insn_cpuid,
	crudasm_intel_insn_daa,
	crudasm_intel_insn_das,
	crudasm_intel_insn_dec,
	crudasm_intel_insn_div,
	crudasm_intel_insn_emms,
	crudasm_intel_insn_enter,
	crudasm_intel_insn_f2xm1,
	crudasm_intel_insn_fcos,
	crudasm_intel_insn_fimul,
	crudasm_intel_insn_fld,
	crudasm_intel_insn_fldcw,
	crudasm_intel_insn_fmulp,
	crudasm_intel_insn_fst,
	crudasm_intel_insn_fstp,
	crudasm_intel_insn_fxtract,
	crudasm_intel_insn_hlt,
	crudasm_intel_insn_idiv,
	crudasm_intel_insn_imul,
	crudasm_intel_insn_in,
	crudasm_intel_insn_inc,
	crudasm_intel_insn_int,
	crudasm_intel_insn_into,
	crudasm_intel_insn_invd,
	crudasm_intel_insn_invept,
	crudasm_intel_insn_invlpg,
	crudasm_intel_insn_invvpid,
	crudasm_intel_insn_ja,
	crudasm_intel_insn_jbe,
	crudasm_intel_insn_jc,
	crudasm_intel_insn_jg,
	crudasm_intel_insn_jge,
	crudasm_intel_insn_jl,
	crudasm_intel_insn_jle,
	crudasm_intel_insn_jmp,
	crudasm_intel_insn_jnc,
	crudasm_intel_insn_jno,
	crudasm_intel_insn_jnp,
	crudasm_intel_insn_jns,
	crudasm_intel_insn_jnz,
	crudasm_intel_insn_jo,
	crudasm_intel_insn_jp,
	crudasm_intel_insn_js,
	crudasm_intel_insn_jz,
	crudasm_intel_insn_lahf,
	crudasm_intel_insn_lar,
	crudasm_intel_insn_ldmxcsr,
	crudasm_intel_insn_lds,
	crudasm_intel_insn_lea,
	crudasm_intel_insn_leave,
	crudasm_intel_insn_les,
	crudasm_intel_insn_lfs,
	crudasm_intel_insn_lgdt,
	crudasm_intel_insn_lgs,
	crudasm_intel_insn_lidt,
	crudasm_intel_insn_lldt,
	crudasm_intel_insn_lmsw,
	crudasm_intel_insn_lsl,
	crudasm_intel_insn_lss,
	crudasm_intel_insn_ltr,
	crudasm_intel_insn_mov,
	crudasm_intel_insn_movaps,
	crudasm_intel_insn_movdqa,
	crudasm_intel_insn_movdqu,
	crudasm_intel_insn_movss,
	crudasm_intel_insn_movsx,
	crudasm_intel_insn_movsxd,
	crudasm_intel_insn_movzx,
	crudasm_intel_insn_mul,
	crudasm_intel_insn_neg,
	crudasm_intel_insn_not,
	crudasm_intel_insn_or,
	crudasm_intel_insn_out,
	crudasm_intel_insn_phaddd,
	crudasm_intel_insn_phaddw,
	crudasm_intel_insn_prefetch,
	crudasm_intel_insn_prefetchw,
	crudasm_intel_insn_rcl,
	crudasm_intel_insn_rcr,
	crudasm_intel_insn_rdmsr,
	crudasm_intel_insn_rdpmc,
	crudasm_intel_insn_rdtsc,
	crudasm_intel_insn_rol,
	crudasm_intel_insn_ror,
	crudasm_intel_insn_rsm,
	crudasm_intel_insn_sahf,
	crudasm_intel_insn_sar,
	crudasm_intel_insn_sbb,
	crudasm_intel_insn_seta,
	crudasm_intel_insn_setbe,
	crudasm_intel_insn_setc,
	crudasm_intel_insn_setg,
	crudasm_intel_insn_setge,
	crudasm_intel_insn_setl,
	crudasm_intel_insn_setle,
	crudasm_intel_insn_setnc,
	crudasm_intel_insn_setno,
	crudasm_intel_insn_setnp,
	crudasm_intel_insn_setns,
	crudasm_intel_insn_setnz,
	crudasm_intel_insn_seto,
	crudasm_intel_insn_setp,
	crudasm_intel_insn_sets,
	crudasm_intel_insn_setz,
	crudasm_intel_insn_sgdt,
	crudasm_intel_insn_shl,
	crudasm_intel_insn_shld,
	crudasm_intel_insn_shr,
	crudasm_intel_insn_shrd,
	crudasm_intel_insn_sidt,
	crudasm_intel_insn_sldt,
	crudasm_intel_insn_smsw,
	crudasm_intel_insn_stc,
	crudasm_intel_insn_std,
	crudasm_intel_insn_sti,
	crudasm_intel_insn_stmxcsr,
	crudasm_intel_insn_str,
	crudasm_intel_insn_sub,
	crudasm_intel_insn_syscall,
	crudasm_intel_insn_sysenter,
	crudasm_intel_insn_sysexit,
	crudasm_intel_insn_sysret,
	crudasm_intel_insn_ud2,
	crudasm_intel_insn_verr,
	crudasm_intel_insn_verw,
	crudasm_intel_insn_vmcall,
	crudasm_intel_insn_vmclear,
	crudasm_intel_insn_vmlaunch,
	crudasm_intel_insn_vmptrld,
	crudasm_intel_insn_vmptrst,
	crudasm_intel_insn_vmread,
	crudasm_intel_insn_vmresume,
	crudasm_intel_insn_vmwrite,
	crudasm_intel_insn_vmxoff,
	crudasm_intel_insn_vmxon,
	crudasm_intel_insn_wait,
	crudasm_intel_insn_wbinvd,
	crudasm_intel_insn_wrmsr,
	crudasm_intel_insn_xadd,
	crudasm_intel_insn_xor,
	crudasm_intel_insncount
};

enum {
	crudasm_intel_etag_no64 = 0x1,
	crudasm_intel_etag_asm_skip = 0x2,
	crudasm_intel_etag_imm64_sx32 = 0x4,
	crudasm_intel_etag_sx_byte = 0x8,
	crudasm_intel_etag_imm64_disp = 0x10,
	crudasm_intel_etag_reg_base = 0x20,
	crudasm_intel_etag_no_rex_w = 0x40,
	crudasm_intel_etag_like_arpl = 0x80,
	crudasm_intel_etag_like_movsxd = 0x100,
	crudasm_intel_etag_reg_rm = 0x200,
	crudasm_intel_etag_is64 = 0x400,
	crudasm_intel_etag_asm_66_if_o16 = 0x800,
	crudasm_intel_etag_has_cc = 0x1000,
	crudasm_intel_etag_relative = 0x2000,
	crudasm_intel_etagcount = 14
};

enum {
	crudasm_intel_itag_ctrlxfer = 0x1,
	crudasm_intel_itag_lockable = 0x2,
	crudasm_intel_itag_repable = 0x4,
	crudasm_intel_itag_repcond = 0x8,
	crudasm_intel_itag_lock_always = 0x10,
	crudasm_intel_itag_fwaitable = 0x20,
	crudasm_intel_itagcount = 6
};

struct crudasm_intel_insn_t {
	const char *alias;
	U4 itags;	// instruction tags
	U4 valid_modes;
	U4 first_encoding_index;
	U4 last_encoding_index;
	U1 numArgs;
	S1 argSizes[CRUDASM_MAX_ASM_ARGS];
};


#ifdef __cplusplus
extern "C" {
#endif

extern struct crudasm_intel_insn_t crudasm_intel_insns[];  /* see out_intel_encoding_table.h */

#ifdef __cplusplus
}
#endif
enum {
	crudasm_intel_mode_real = 0x1,
	crudasm_intel_mode_protected = 0x2,
	crudasm_intel_mode_vm86 = 0x4,
	crudasm_intel_mode_smm = 0x8,
	crudasm_intel_mode_pmode64 = 0x10,
	crudasm_intel_mode_compat = 0x20,
	crudasm_intel_modecount
};

