// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <linux/cpu.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/sched/debug.h>
#include <linux/sched/signal.h>
#include <linux/signal.h>
#include <linux/kdebug.h>
#include <linux/uaccess.h>
#include <linux/kprobes.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/irq.h>
#include <linux/kexec.h>

#include <asm/asm-prototypes.h>
#include <asm/bug.h>
#include <asm/csr.h>
#include <asm/processor.h>
#include <asm/ptrace.h>
#include <asm/thread_info.h>
typedef unsigned long ulong;

static inline unsigned long* regs_get_register_p(struct pt_regs *regs,
					      unsigned int offset)
{
	static unsigned long zero = 0;
	if (offset == 0)
		return &zero;
	return &(((unsigned long*)regs)[offset]);
}
ulong noinline _csr_read(void){
	ulong ret;
	asm volatile ("csrr %0, 0x0" : "=r"(ret):);
	return ret;
}

void noinline _write_csr(ulong val){
	asm volatile ("csrw 0x0, %0" : : "r"(val));
}

ulong csr_rd(ulong csr){
	ulong* tmp = ((ulong*)_csr_read)+3;
	*tmp &= ~(0xfff<<20);
	*tmp |= (csr<<20);
	return _csr_read();
}


void csr_wr(ulong csr, ulong val){
	ulong* tmp = ((ulong*)_write_csr)+3;
	*tmp &= ~(0xfff<<20);
	*tmp |= (csr<<20);
	_write_csr(val);
}

static noinline void do_csr(struct pt_regs *regs){
	ulong csrval;
	ulong op = *(ulong*)regs->epc;
	ulong csr = (op>>20)&0xfff;
	ulong microop = (op>>12)&0x7;
	ulong rs1imm = (op >> 15) & 0x1f;
	ulong* rsd = regs_get_register_p(regs,(op >> 7) & 0x1f);
	if(microop == 0)
		return;
	/*The microop isnt imm */
	if(!(microop>>2))
		rs1imm = *regs_get_register_p(regs,rs1imm);
	if(csr == 0x300){
		csrval = regs->status;
	}
	else
		csrval = csr_rd(csr);
	if((op >> 7) & 0x1f)
		*rsd = csrval;
	switch( microop & 0x3){
		case 0b01: csrval = rs1imm; break; //CSRW
		case 0b10: csrval |= rs1imm | 0x80; break; //CSRRS
		case 0b11: csrval &= ~rs1imm; break; //CSRRC
	}
	if(csr == 0x300)
		regs->status=csrval;
	else
		csr_wr(csr,csrval);

}

static void do_atomic(struct pt_regs *regs){
	ulong op = *(uint32_t*)regs->epc;
	ulong irmid = (op>>27)&0x1f;
	ulong rs1 = *regs_get_register_p(regs,(op >> 15) & 0x1f);
	ulong rs2 = *regs_get_register_p(regs,(op >> 20) & 0x1f);
	ulong* rsd = regs_get_register_p(regs,(op >> 7) & 0x1f);
	ulong* dst = (ulong*)rs1;
	if((op >> 7) & 0x1f)
		*rsd = *dst;
	switch( irmid ){
		case 0b00010: break; //LR.W
		case 0b00011: {
			if((op >> 7) & 0x1f)
				*rsd = 0;
			*dst = rs2;
			break; //SC.W (Lie and always say it's good)
		}
		case 0b00001: *dst = rs2; break; //AMOSWAP.W
		case 0b00000: *dst += rs2; break; //AMOADD.W
		case 0b00100: *dst ^= rs2; break; //AMOXOR.W
		case 0b01100: *dst &= rs2; break; //AMOAND.W
		case 0b01000: *dst |= rs2; break; //AMOOR.W
		default: break;
	}

}

static int do_exinsn(struct pt_regs *regs){
	uint32_t op = *(uint32_t*)regs->epc;
	switch (op & 0x7f) {
		case 0b0101111: do_atomic(regs); return 1;
		case 0b1110011: do_csr(regs); return 1; //CSRRCI
		default: return 0;
	}
}

int show_unhandled_signals = 1;

static DEFINE_SPINLOCK(die_lock);

void die(struct pt_regs *regs, const char *str)
{
	static int die_counter;
	int ret;

	oops_enter();

	spin_lock_irq(&die_lock);
	console_verbose();
	bust_spinlocks(1);

	pr_emerg("%s [#%d]\n", str, ++die_counter);
	print_modules();
	show_regs(regs);

	ret = notify_die(DIE_OOPS, str, regs, 0, regs->cause, SIGSEGV);

	if (regs && kexec_should_crash(current))
		crash_kexec(regs);

	bust_spinlocks(0);
	add_taint(TAINT_DIE, LOCKDEP_NOW_UNRELIABLE);
	spin_unlock_irq(&die_lock);
	oops_exit();

	if (in_interrupt())
		panic("Fatal exception in interrupt");
	if (panic_on_oops)
		panic("Fatal exception");
	if (ret != NOTIFY_STOP)
		make_task_dead(SIGSEGV);
}

void do_trap(struct pt_regs *regs, int signo, int code, unsigned long addr)
{
	struct task_struct *tsk = current;

	if (show_unhandled_signals && unhandled_signal(tsk, signo)
	    && printk_ratelimit()) {
		pr_info("%s[%d]: unhandled signal %d code 0x%x at 0x" REG_FMT,
			tsk->comm, task_pid_nr(tsk), signo, code, addr);
		print_vma_addr(KERN_CONT " in ", instruction_pointer(regs));
		pr_cont("\n");
		__show_regs(regs);
	}

	force_sig_fault(signo, code, (void __user *)addr);
}

static void do_trap_error(struct pt_regs *regs, int signo, int code,
	unsigned long addr, const char *str)
{
	current->thread.bad_cause = regs->cause;

	if (user_mode(regs)) {
		do_trap(regs, signo, code, addr);
	} else {
		/* If illegal instruction and we can emulate, then
		 * we need to emulate and skip the instruction.
		 */
		if(code == ILL_ILLOPC && do_exinsn(regs)){
			regs->epc+=4;
			regs->badaddr+=4;
			return;
		}
		if (!fixup_exception(regs))
			die(regs, str);
	}
}

#if defined(CONFIG_XIP_KERNEL) && defined(CONFIG_RISCV_ALTERNATIVE)
#define __trap_section		__section(".xip.traps")
#else
#define __trap_section
#endif
#define DO_ERROR_INFO(name, signo, code, str)				\
asmlinkage __visible __trap_section void name(struct pt_regs *regs)	\
{									\
	do_trap_error(regs, signo, code, regs->epc, "Oops - " str);	\
}

DO_ERROR_INFO(do_trap_unknown,
	SIGILL, ILL_ILLTRP, "unknown exception");
DO_ERROR_INFO(do_trap_insn_misaligned,
	SIGBUS, BUS_ADRALN, "instruction address misaligned");
DO_ERROR_INFO(do_trap_insn_fault,
	SIGSEGV, SEGV_ACCERR, "instruction access fault");
DO_ERROR_INFO(do_trap_insn_illegal,
	SIGILL, ILL_ILLOPC, "illegal instruction");
DO_ERROR_INFO(do_trap_load_fault,
	SIGSEGV, SEGV_ACCERR, "load access fault");
#ifndef CONFIG_RISCV_M_MODE
DO_ERROR_INFO(do_trap_load_misaligned,
	SIGBUS, BUS_ADRALN, "Oops - load address misaligned");
DO_ERROR_INFO(do_trap_store_misaligned,
	SIGBUS, BUS_ADRALN, "Oops - store (or AMO) address misaligned");
#else
int handle_misaligned_load(struct pt_regs *regs);
int handle_misaligned_store(struct pt_regs *regs);

asmlinkage void __trap_section do_trap_load_misaligned(struct pt_regs *regs)
{
	if (!handle_misaligned_load(regs))
		return;
	do_trap_error(regs, SIGBUS, BUS_ADRALN, regs->epc,
		      "Oops - load address misaligned");
}

asmlinkage void __trap_section do_trap_store_misaligned(struct pt_regs *regs)
{
	if (!handle_misaligned_store(regs))
		return;
	do_trap_error(regs, SIGBUS, BUS_ADRALN, regs->epc,
		      "Oops - store (or AMO) address misaligned");
}
#endif
DO_ERROR_INFO(do_trap_store_fault,
	SIGSEGV, SEGV_ACCERR, "store (or AMO) access fault");
DO_ERROR_INFO(do_trap_ecall_u,
	SIGILL, ILL_ILLTRP, "environment call from U-mode");
DO_ERROR_INFO(do_trap_ecall_s,
	SIGILL, ILL_ILLTRP, "environment call from S-mode");
DO_ERROR_INFO(do_trap_ecall_m,
	SIGILL, ILL_ILLTRP, "environment call from M-mode");

static inline unsigned long get_break_insn_length(unsigned long pc)
{
	bug_insn_t insn;

	if (get_kernel_nofault(insn, (bug_insn_t *)pc))
		return 0;

	return GET_INSN_LENGTH(insn);
}

asmlinkage __visible __trap_section void do_trap_break(struct pt_regs *regs)
{
#ifdef CONFIG_KPROBES
	if (kprobe_single_step_handler(regs))
		return;

	if (kprobe_breakpoint_handler(regs))
		return;
#endif
#ifdef CONFIG_UPROBES
	if (uprobe_single_step_handler(regs))
		return;

	if (uprobe_breakpoint_handler(regs))
		return;
#endif
	current->thread.bad_cause = regs->cause;

	if (user_mode(regs))
		force_sig_fault(SIGTRAP, TRAP_BRKPT, (void __user *)regs->epc);
#ifdef CONFIG_KGDB
	else if (notify_die(DIE_TRAP, "EBREAK", regs, 0, regs->cause, SIGTRAP)
								== NOTIFY_STOP)
		return;
#endif
	else if (report_bug(regs->epc, regs) == BUG_TRAP_TYPE_WARN)
		regs->epc += get_break_insn_length(regs->epc);
	else
		die(regs, "Kernel BUG");
}
NOKPROBE_SYMBOL(do_trap_break);

#ifdef CONFIG_GENERIC_BUG
int is_valid_bugaddr(unsigned long pc)
{
	bug_insn_t insn;

	if (pc < VMALLOC_START)
		return 0;
	if (get_kernel_nofault(insn, (bug_insn_t *)pc))
		return 0;
	if ((insn & __INSN_LENGTH_MASK) == __INSN_LENGTH_32)
		return (insn == __BUG_INSN_32);
	else
		return ((insn & __COMPRESSED_INSN_MASK) == __BUG_INSN_16);
}
#endif /* CONFIG_GENERIC_BUG */

#ifdef CONFIG_VMAP_STACK
static DEFINE_PER_CPU(unsigned long [OVERFLOW_STACK_SIZE/sizeof(long)],
		overflow_stack)__aligned(16);
/*
 * shadow stack, handled_ kernel_ stack_ overflow(in kernel/entry.S) is used
 * to get per-cpu overflow stack(get_overflow_stack).
 */
long shadow_stack[SHADOW_OVERFLOW_STACK_SIZE/sizeof(long)];
asmlinkage unsigned long get_overflow_stack(void)
{
	return (unsigned long)this_cpu_ptr(overflow_stack) +
		OVERFLOW_STACK_SIZE;
}

asmlinkage void handle_bad_stack(struct pt_regs *regs)
{
	unsigned long tsk_stk = (unsigned long)current->stack;
	unsigned long ovf_stk = (unsigned long)this_cpu_ptr(overflow_stack);

	console_verbose();

	pr_emerg("Insufficient stack space to handle exception!\n");
	pr_emerg("Task stack:     [0x%016lx..0x%016lx]\n",
			tsk_stk, tsk_stk + THREAD_SIZE);
	pr_emerg("Overflow stack: [0x%016lx..0x%016lx]\n",
			ovf_stk, ovf_stk + OVERFLOW_STACK_SIZE);

	__show_regs(regs);
	panic("Kernel stack overflow");

	for (;;)
		wait_for_interrupt();
}
#endif
