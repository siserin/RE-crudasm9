// out_intel_disasm_nasm.h  (note: this file was automatically generated -- do not edit!)
// Copyright (C) 2012 Willow Schlanger. All rights reserved.

static int crudasm_intel_disasm_special_nasm(struct crudasm_intel_disasm_context_t *context, U4 insn) {
	switch(insn) {
		case crudasm_intel_insn__aad:
			return intel_nasm_insn__aad(context);
		case crudasm_intel_insn__aam:
			return intel_nasm_insn__aam(context);
		case crudasm_intel_insn__bt_mem_reg:
			{
				ixdis1_write(context, "bt");
				ixdis0_write_any_args(context);
				return 1;	// handled
			}
		case crudasm_intel_insn__bt_other:
			{
				ixdis1_write(context, "bt");
				ixdis0_write_any_args(context);
				return 1;	// handled
			}
		case crudasm_intel_insn__callfd:
			return intel_nasm_insn__callfd(context);
		case crudasm_intel_insn__callfi:
			return intel_nasm_insn__callfi(context);
		case crudasm_intel_insn__calli:
			return intel_nasm_insn__calli(context);
		case crudasm_intel_insn__cmps:
			return intel_nasm_insn__cmps(context);
		case crudasm_intel_insn__cmpxchgxb:
			return intel_nasm_insn__cmpxchgxb(context);
		case crudasm_intel_insn__cmul2:
			return intel_nasm_insn__cmul2(context);
		case crudasm_intel_insn__cmul3:
			return intel_nasm_insn__cmul3(context);
		case crudasm_intel_insn__divb:
			return intel_nasm_insn__divb(context);
		case crudasm_intel_insn__fmul1:
			return intel_nasm_insn__fmul1(context);
		case crudasm_intel_insn__fmul2:
			return intel_nasm_insn__fmul2(context);
		case crudasm_intel_insn__fxch:
			return intel_nasm_insn__fxch(context);
		case crudasm_intel_insn__fxrstor:
			return intel_nasm_insn__fxrstor(context);
		case crudasm_intel_insn__fxsave:
			return intel_nasm_insn__fxsave(context);
		case crudasm_intel_insn__idivb:
			return intel_nasm_insn__idivb(context);
		case crudasm_intel_insn__imulb:
			return intel_nasm_insn__imulb(context);
		case crudasm_intel_insn__ins:
			return intel_nasm_insn__ins(context);
		case crudasm_intel_insn__int3:
			return intel_nasm_insn__int3(context);
		case crudasm_intel_insn__iret:
			return intel_nasm_insn__iret(context);
		case crudasm_intel_insn__jmpfd:
			return intel_nasm_insn__jmpfd(context);
		case crudasm_intel_insn__jmpfi:
			return intel_nasm_insn__jmpfi(context);
		case crudasm_intel_insn__jmpi:
			return intel_nasm_insn__jmpi(context);
		case crudasm_intel_insn__jrcxz:
			return intel_nasm_insn__jrcxz(context);
		case crudasm_intel_insn__lods:
			return intel_nasm_insn__lods(context);
		case crudasm_intel_insn__loop:
			return intel_nasm_insn__loop(context);
		case crudasm_intel_insn__loopnz:
			return intel_nasm_insn__loopnz(context);
		case crudasm_intel_insn__loopz:
			return intel_nasm_insn__loopz(context);
		case crudasm_intel_insn__movcr:
			return intel_nasm_insn__movcr(context);
		case crudasm_intel_insn__movdr:
			return intel_nasm_insn__movdr(context);
		case crudasm_intel_insn__movs:
			return intel_nasm_insn__movs(context);
		case crudasm_intel_insn__movsd2:
			return intel_nasm_insn__movsd2(context);
		case crudasm_intel_insn__movsrv:
			return intel_nasm_insn__movsrv(context);
		case crudasm_intel_insn__movvsr:
			return intel_nasm_insn__movvsr(context);
		case crudasm_intel_insn__mulb:
			return intel_nasm_insn__mulb(context);
		case crudasm_intel_insn__nopmb:
			return intel_nasm_insn__nopmb(context);
		case crudasm_intel_insn__outs:
			return intel_nasm_insn__outs(context);
		case crudasm_intel_insn__pop:
			return intel_nasm_insn__pop(context);
		case crudasm_intel_insn__popa:
			return intel_nasm_insn__popa(context);
		case crudasm_intel_insn__popf:
			return intel_nasm_insn__popf(context);
		case crudasm_intel_insn__popsr:
			return intel_nasm_insn__popsr(context);
		case crudasm_intel_insn__push:
			return intel_nasm_insn__push(context);
		case crudasm_intel_insn__pusha:
			return intel_nasm_insn__pusha(context);
		case crudasm_intel_insn__pushf:
			return intel_nasm_insn__pushf(context);
		case crudasm_intel_insn__pushsr:
			return intel_nasm_insn__pushsr(context);
		case crudasm_intel_insn__ret:
			return intel_nasm_insn__ret(context);
		case crudasm_intel_insn__retf:
			return intel_nasm_insn__retf(context);
		case crudasm_intel_insn__retfnum:
			return intel_nasm_insn__retfnum(context);
		case crudasm_intel_insn__retnum:
			return intel_nasm_insn__retnum(context);
		case crudasm_intel_insn__sal:
			return intel_nasm_insn__sal(context);
		case crudasm_intel_insn__scas:
			return intel_nasm_insn__scas(context);
		case crudasm_intel_insn__stos:
			return intel_nasm_insn__stos(context);
		case crudasm_intel_insn__sxacc:
			return intel_nasm_insn__sxacc(context);
		case crudasm_intel_insn__sxdax:
			return intel_nasm_insn__sxdax(context);
		case crudasm_intel_insn__test:
			return intel_nasm_insn__test(context);
		case crudasm_intel_insn__uint1:
			return intel_nasm_insn__uint1(context);
		case crudasm_intel_insn__usalc:
			return intel_nasm_insn__usalc(context);
		case crudasm_intel_insn__xchg:
			return intel_nasm_insn__xchg(context);
		case crudasm_intel_insn__xlat:
			return intel_nasm_insn__xlat(context);
		case crudasm_intel_insn_cmpxchg:
			return intel_nasm_insn_cmpxchg(context);
		case crudasm_intel_insn_in:
			return intel_nasm_insn_in(context);
		case crudasm_intel_insn_out:
			return intel_nasm_insn_out(context);
		default: break;
	}
	return 0;	// not handled specially
}

