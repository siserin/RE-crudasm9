// intel_disasm_nasm.h - Copyright (C) 2011,2012 Willow Schlanger. All rights reserved.

int intel_nasm_insn__fmul1(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "fmul");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__fmul2(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "fmul");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__aad(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "aad");
			ixdis1_maybe_write_space_args_imm32(context, 0x0a);
	return 1;	// handled
}

int intel_nasm_insn__aam(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "aam");
			ixdis1_maybe_write_space_args_imm32(context, 0x0a);
	return 1;	// handled
}

int intel_nasm_insn__callfd(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "call");
			ixdis0_space(context);
			ixdis0_write_far_imm(context);
	return 1;	// handled
}

int intel_nasm_insn__callfi(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "call");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__calli(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "call");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__cmps(struct crudasm_intel_disasm_context_t *context) {
			ixdis0_write_seg_reg(context);
			ixdis0_write_repcc(context);
			ixdis1_write(context, "cmps");
			ixdis1_write_size_suffix_argsize(context, 0);
	return 1;	// handled
}

int intel_nasm_insn__cmpxchgxb(struct crudasm_intel_disasm_context_t *context) {
			ixdis2_write_osz64(context, "cmpxchg16b", "cmpxchg8b");
			ixdis0_space(context);
			ixdis2_write_arg(context, 0, 0);
	return 1;	// handled
}

int intel_nasm_insn__cmul2(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "imul");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__cmul3(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "imul");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__divb(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "div");
			ixdis0_space(context);
			ixdis3_write_arg(context, 0, 1, 1);
	return 1;	// handled
}

int intel_nasm_insn__fxch(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "fxch");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__fxrstor(struct crudasm_intel_disasm_context_t *context) {
			ixdis2_write_osz64(context, "fxrstor64", "fxrstor");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__fxsave(struct crudasm_intel_disasm_context_t *context) {
			ixdis2_write_osz64(context, "fxsave64", "fxsave");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__idivb(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "idiv");
			ixdis0_space(context);
			ixdis3_write_arg(context, 0, 1, 1);
	return 1;	// handled
}

int intel_nasm_insn__imulb(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "imul");
			ixdis0_space(context);
			ixdis3_write_arg(context, 0, 1, 1);
	return 1;	// handled
}

int intel_nasm_insn__ins(struct crudasm_intel_disasm_context_t *context) {
			ixdis0_write_rep(context);
			ixdis1_write(context, "ins");
			ixdis1_write_size_suffix_argsize(context, 0);
	return 1;	// handled
}

int intel_nasm_insn__int3(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "int3");
	return 1;	// handled
}

int intel_nasm_insn__iret(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "iret");
			ixdis0_write_size_suffix_osz(context);
	return 1;	// handled
}

int intel_nasm_insn__jmpfd(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "jmp");
			ixdis0_space(context);
			ixdis0_write_far_imm(context);
	return 1;	// handled
}

int intel_nasm_insn__jmpfi(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "jmp");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__jmpi(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "jmp");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__jrcxz(struct crudasm_intel_disasm_context_t *context) {
			ixdis0_write_jrcxz_o16_o32_o64(context);
			ixdis3_write_asz(context, "jcxz", "jecxz", "jrcxz");
			ixdis0_space(context);
			ixdis4_write_arg(context, 0, 1, 0, 1);
	return 1;	// handled
}

int intel_nasm_insn__lods(struct crudasm_intel_disasm_context_t *context) {
			ixdis0_write_seg_reg(context);
			ixdis0_write_rep(context);
			ixdis1_write(context, "lods");
			ixdis1_write_size_suffix_argsize(context, 0);
	return 1;	// handled
}

int intel_nasm_insn__loop(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "loop");
			ixdis0_space(context);
			ixdis4_write_arg(context, 0, 1, 0, 1);
	return 1;	// handled
}

int intel_nasm_insn__loopnz(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "loopnz");
			ixdis0_space(context);
			ixdis4_write_arg(context, 0, 1, 0, 1);
	return 1;	// handled
}

int intel_nasm_insn__loopz(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "loopz");
			ixdis0_space(context);
			ixdis4_write_arg(context, 0, 1, 0, 1);
	return 1;	// handled
}

int intel_nasm_insn__movcr(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "mov");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__movdr(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "mov");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__movs(struct crudasm_intel_disasm_context_t *context) {
			ixdis0_write_seg_reg(context);
			ixdis0_write_rep(context);
			ixdis1_write(context, "movs");
			ixdis1_write_size_suffix_argsize(context, 0);
	return 1;	// handled
}

int intel_nasm_insn__movsd2(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "movsd");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__movsrv(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "mov");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__movvsr(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "mov");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__mulb(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "mul");
			ixdis0_space(context);
			ixdis3_write_arg(context, 0, 1, 1);
	return 1;	// handled
}

int intel_nasm_insn__nopmb(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "nop");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__outs(struct crudasm_intel_disasm_context_t *context) {
			ixdis0_write_seg_reg(context);
			ixdis0_write_rep(context);
			ixdis1_write(context, "outs");
			ixdis1_write_size_suffix_argsize(context, 1);
	return 1;	// handled
}

int intel_nasm_insn__pop(struct crudasm_intel_disasm_context_t *context) {
			ixdis0_write_stack_o16_o32_o64(context);
			ixdis1_write(context, "pop");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__popa(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "popa");
			ixdis0_write_size_suffix_osz(context);
	return 1;	// handled
}

int intel_nasm_insn__popf(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "popf");
			ixdis0_write_size_suffix_osz(context);
	return 1;	// handled
}

int intel_nasm_insn__popsr(struct crudasm_intel_disasm_context_t *context) {
			ixdis0_write_stack_o16_o32_o64(context);
			ixdis1_write(context, "pop");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__push(struct crudasm_intel_disasm_context_t *context) {
			ixdis0_write_stack_o16_o32_o64(context);
			ixdis1_write(context, "push");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__pusha(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "pusha");
			ixdis0_write_size_suffix_osz(context);
	return 1;	// handled
}

int intel_nasm_insn__pushf(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "pushf");
			ixdis0_write_size_suffix_osz(context);
	return 1;	// handled
}

int intel_nasm_insn__pushsr(struct crudasm_intel_disasm_context_t *context) {
			ixdis0_write_stack_o16_o32_o64(context);
			ixdis1_write(context, "push");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__ret(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "ret");
	return 1;	// handled
}

int intel_nasm_insn__retf(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "retf");
	return 1;	// handled
}

int intel_nasm_insn__retfnum(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "retf");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__retnum(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "ret");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__sal(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "shl");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__scas(struct crudasm_intel_disasm_context_t *context) {
			ixdis0_write_repcc(context);
			ixdis1_write(context, "scas");
			ixdis1_write_size_suffix_argsize(context, 0);
	return 1;	// handled
}

int intel_nasm_insn__stos(struct crudasm_intel_disasm_context_t *context) {
			ixdis0_write_rep(context);
			ixdis1_write(context, "stos");
			ixdis1_write_size_suffix_argsize(context, 0);
	return 1;	// handled
}

int intel_nasm_insn__sxacc(struct crudasm_intel_disasm_context_t *context) {
			ixdis3_write_osz(context, "cbw", "cwde", "cdqe");
	return 1;	// handled
}

int intel_nasm_insn__sxdax(struct crudasm_intel_disasm_context_t *context) {
			ixdis3_write_osz(context, "cwd", "cdq", "cqo");
	return 1;	// handled
}

int intel_nasm_insn__test(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "test");
			ixdis0_space(context);
			ixdis0_write_args(context);
	return 1;	// handled
}

int intel_nasm_insn__uint1(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "int1");
	return 1;	// handled
}

int intel_nasm_insn__usalc(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "salc");
	return 1;	// handled
}

int intel_nasm_insn__xchg(struct crudasm_intel_disasm_context_t *context) {
			ixdis3_do_nop_xchg(context, "nop", "xchg", 1);
	return 1;	// handled
}

int intel_nasm_insn__xlat(struct crudasm_intel_disasm_context_t *context) {
			ixdis0_write_xlat_o16_o32_o64(context);
			ixdis0_write_seg_reg(context);
			ixdis1_write(context, "xlatb");
	return 1;	// handled
}

int intel_nasm_insn_cmpxchg(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "cmpxchg");
			ixdis0_space(context);
			ixdis1_write_arg(context, 0);
			ixdis0_comma(context);
			ixdis1_write_arg(context, 1);
	return 1;	// handled
}

int intel_nasm_insn_in(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "in");
			ixdis0_space(context);
			ixdis2_write_arg(context, 0, 0);
			ixdis0_comma(context);
			ixdis2_write_arg(context, 1, 0);
	return 1;	// handled
}

int intel_nasm_insn_out(struct crudasm_intel_disasm_context_t *context) {
			ixdis1_write(context, "out");
			ixdis0_space(context);
			ixdis2_write_arg(context, 0, 0);
			ixdis0_comma(context);
			ixdis2_write_arg(context, 1, 0);
	return 1;	// handled
}
