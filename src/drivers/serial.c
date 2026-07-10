#include <kernel/serial.h>
#include <kernel/panic.h>
#include <kernel/printf.h>
#include <kernel/mm.h>
#include <arch/io.h>
#include <arch/spinlock.h>
#include <arch/csr.h>
#include <arch/plic.h>

#define SERIAL_VA ((void *)(KERNEL_DIRECT_MAP_START + (u64)SERIAL_BASE))

#define SERIAL_BUF_SIZE 256
static char serial_buf[SERIAL_BUF_SIZE];
static size_t serial_buf_len = 0;
static struct spinlock serial_lock;

static inline u8 serial_reg_read(u64 reg)
{
	return ioread8(SERIAL_VA + reg);
}

static inline void serial_reg_write(u8 val, u64 reg)
{
	iowrite8(val, SERIAL_VA + reg);
}

void serial_init()
{
	spin_init(&serial_lock);
	serial_buf_len = 0;

	serial_reg_write(0x00, SERIAL_IER);

	serial_reg_write(SERIAL_FCR_FIFO_ENABLE |
	                 SERIAL_FCR_RX_FIFO_CLEAR |
	                 SERIAL_FCR_TX_FIFO_CLEAR, SERIAL_FCR);

	serial_reg_write(0x03, SERIAL_LCR);
}

void serial_irq_enable()
{
	serial_reg_write(SERIAL_IER_ERBFI, SERIAL_IER);

	plic_irq_set_priority(IRQ_SERIAL, 1);
	plic_hart_enable_irq(0, IRQ_SERIAL);
	plic_hart_set_threshold(0, 0);

	csr_set(CSR_SIE, CSR_SIE_SEIE);
}

void serial_irq_disable()
{
	serial_reg_write(0x00, SERIAL_IER);
	csr_clear(CSR_SIE, CSR_SIE_SEIE);
}

void serial_irq()
{
	while (serial_reg_read(SERIAL_LSR) & SERIAL_LSR_DTR) {
		char c = (char)serial_reg_read(SERIAL_RBR);

		u64 flags = hart_irq_save();
		if (serial_buf_len < SERIAL_BUF_SIZE) {
			serial_buf[serial_buf_len++] = c;
		}
		hart_irq_restore(flags);
	}
}

size_t serial_read(char *buf)
{
	u64 flags = hart_irq_save();
	size_t len = serial_buf_len;
	for (size_t i = 0; i < len; i++)
		buf[i] = serial_buf[i];
	serial_buf_len = 0;
	hart_irq_restore(flags);
	return len;
}

void serial_putc(char c)
{
	while (!(serial_reg_read(SERIAL_LSR) & SERIAL_LSR_THRE))
		;
	serial_reg_write((u8)c, SERIAL_THR);
}

void serial_puts(char *str)
{
	while (*str)
		serial_putc(*str++);
}
