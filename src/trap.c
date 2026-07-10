#include <kernel/trap.h>
#include <kernel/panic.h>
#include <kernel/printf.h>
#include <arch/csr.h>
#include <arch/timer.h>
#include <arch/plic.h>
#include <kernel/serial.h>

extern void trap_entry();

void handle_irq()
{
	u64 scause = csr_read(CSR_SCAUSE);
	u64 cause = scause & ~TRAP_IRQ_BIT;

	if (scause == TRAP_TIMER_IRQ) {
		timer_irq();
	} else if (scause == TRAP_EXTERNAL_IRQ) {
		u32 irq = plic_hart_claim_irq(0);
		if (irq == IRQ_SERIAL) {
			serial_irq();
		}
		/* acabouy */
		plic_hart_complete_irq(0, irq);
	} else {
		warn("handle_irq: unhandled IRQ cause=%lu\n", cause);
	}
}

void handle_exception()
{
	u64 scause = csr_read(CSR_SCAUSE);
	u64 stval  = csr_read(CSR_STVAL);
	u64 sepc   = csr_read(CSR_SEPC);

	switch (scause) {
	case EXCEPTION_INST_PAGE_FAULT:
		panic("Instruction page fault at sepc=0x%lx stval=0x%lx\n", sepc, stval);
		break;
	case EXCEPTION_LOAD_PAGE_FAULT:
		panic("Load page fault at sepc=0x%lx stval=0x%lx\n", sepc, stval);
		break;
	case EXCEPTION_STORE_PAGE_FAULT:
		panic("Store page fault at sepc=0x%lx stval=0x%lx\n", sepc, stval);
		break;
	case EXCEPTION_INST_ACCESS_FAULT:
		panic("Instruction access fault at sepc=0x%lx stval=0x%lx\n", sepc, stval);
		break;
	case EXCEPTION_LOAD_ACCESS_FAULT:
		panic("Load access fault at sepc=0x%lx stval=0x%lx\n", sepc, stval);
		break;
	case EXCEPTION_STORE_ACCESS_FAULT:
		panic("Store access fault at sepc=0x%lx stval=0x%lx\n", sepc, stval);
		break;
	default:
		panic("Unhandled exception: scause=0x%lx sepc=0x%lx stval=0x%lx\n", scause, sepc, stval);
		break;
	}
}

void trap_setup()
{
	csr_write(CSR_STVEC, (u64)trap_entry);
	csr_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
}

void handle_trap()
{
	u64 scause = csr_read(CSR_SCAUSE);

	if (scause & TRAP_IRQ_BIT) {
		handle_irq();
	} else {
		handle_exception();
	}
}

void hart_irq_enable()
{
	csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE);
}

u64 hart_irq_save()
{
	u64 flags = csr_read(CSR_SSTATUS) & CSR_SSTATUS_SIE;
	csr_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
	return flags;
}

void hart_irq_restore(u64 flags)
{
	if (flags & CSR_SSTATUS_SIE)
		csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE);
}

void hart_irq_disable()
{
	csr_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
}
