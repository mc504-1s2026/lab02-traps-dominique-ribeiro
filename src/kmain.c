#include <kernel/printf.h>
#include <kernel/mm.h>
#include <arch/timer.h>
#include <kernel/trap.h>
#include <kernel/serial.h>
#include <kernel/string.h>

extern int _hartid[];

#define CMD_BUF_SIZE 128

static void shell_process_command(char *cmd, size_t len)
{
	cmd[len] = '\0';

	/* remove \r \n */
	while (len > 0 && (cmd[len-1] == '\r' || cmd[len-1] == '\n')) {
		cmd[--len] = '\0';
	}

	if (len == 0) {
		serial_puts("> ");
		return;
	}

	if (strncmp(cmd, "uptime", 6) == 0 && len == 6) {
		u64 secs = timer_uptime_secs();
		char buf[32];
		int i = 0;
		if (secs == 0) {
			buf[i++] = '0';
		} else {
			u64 tmp = secs;
			int start = i;
			while (tmp > 0) {
				buf[i++] = '0' + (tmp % 10);
				tmp /= 10;
			}
			/* inverte */
			int end = i - 1;
			while (start < end) {
				char t = buf[start];
				buf[start] = buf[end];
				buf[end] = t;
				start++; end--;
			}
		}
		buf[i++] = 's';
		buf[i++] = '\n';
		buf[i] = '\0';
		serial_puts(buf);
	} else if (strncmp(cmd, "echo ", 5) == 0) {
		serial_puts(cmd + 5);
		serial_puts("\n");
	} else if (strncmp(cmd, "alarm ", 6) == 0) {
		u64 secs = strtou64(cmd + 6, 10);
		timer_set_alarm(secs);
	} else {
		serial_puts("unknown command: ");
		serial_puts(cmd);
		serial_puts("\n");
	}

	serial_puts("> ");
}

void kmain()
{
	printk_set_level(LOG_DEBUG);
	info("entered S-mode\n");
	info("booting on hart %d\n", _hartid[0]);
	info("setting up virtual memory...\n");
	vm_init();

	info("enabling traps...\n");
	trap_setup();
	info("enabling timer...\n");
	timer_irq_enable();
	info("enabling serial...\n");
	serial_init();
	serial_irq_enable();


	serial_puts("> ");

	static char cmd_buf[CMD_BUF_SIZE];
	static size_t cmd_len = 0;
	char read_buf[SERIAL_BUF_SIZE];

	while (1) {
		size_t n = serial_read(read_buf);
		for (size_t i = 0; i < n; i++) {
			char c = read_buf[i];

			serial_putc(c);

			if (c == '\r' || c == '\n') {
				serial_putc('\n');
				shell_process_command(cmd_buf, cmd_len);
				cmd_len = 0;
			} else if (c == '\b' || c == 127) {
				if (cmd_len > 0) cmd_len--;
			} else {
				if (cmd_len < CMD_BUF_SIZE - 1)
					cmd_buf[cmd_len++] = c;
			}
		}
	}
}
