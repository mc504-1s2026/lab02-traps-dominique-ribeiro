#include <arch/timer.h>
#include <arch/csr.h>
#include <kernel/panic.h>
#include <kernel/printf.h>
#include <kernel/serial.h>

static u64 boot_time = 0;

static u64 alarm_time = 0;
static int alarm_pending = 0;

u64 timer_read()
{
	return csr_read(CSR_TIME);
}

void timer_irq_enable()
{
	boot_time = csr_read(CSR_TIME);

	csr_set(CSR_SIE, CSR_SIE_STIE);

	csr_write(CSR_STIMECMP, boot_time + TIMER_FREQ);

	csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE);
}

void timer_irq_disable()
{
	csr_clear(CSR_SIE, CSR_SIE_STIE);
}

void timer_set_alarm(u64 secs)
{
	alarm_time = csr_read(CSR_TIME) + secs * TIMER_FREQ;
	alarm_pending = 1;
	u64 next = csr_read(CSR_TIME) + TIMER_FREQ;
	if (alarm_time < next)
		next = alarm_time;
	csr_write(CSR_STIMECMP, next);
}

void timer_irq()
{
	u64 now = csr_read(CSR_TIME);

	if (alarm_pending && now >= alarm_time) {
		alarm_pending = 0;
		serial_puts("alarm\n");
	}

	u64 next = now + TIMER_FREQ;
	if (alarm_pending && alarm_time < next)
		next = alarm_time;

	csr_write(CSR_STIMECMP, next);
}

u64 timer_uptime_secs()
{
	return (csr_read(CSR_TIME) - boot_time) / TIMER_FREQ;
}
