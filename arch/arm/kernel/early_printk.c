// SPDX-License-Identifier: GPL-2.0-only
/*
 *  linux/arch/arm/kernel/early_printk.c
 *
 *  Copyright (C) 2009 Sascha Hauer <s.hauer@pengutronix.de>
 */

#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/io.h>
#define STAT_TDRE	(1 << 23)
struct lpuart_fsl_reg32 {
	u32 verid;
	u32 param;
	u32 global;
	u32 pincfg;
	u32 baud;
	u32 stat;
	u32 ctrl;
	u32 data;
	u32 match;
	u32 modir;
	u32 fifo;
	u32 water;
};
static void printc(const char c){
	u32 stat;
	struct lpuart_fsl_reg32 *base = 0x40184000;
	while (true) {
		stat = readl(&base->stat);
		if ((stat & STAT_TDRE))
			break;
	}
	writel(c, &base->data);
}

extern void printascii(const char *);
static void printasciin(const char *s){
	while(*s){
		printc(*s);
		if(*s == '\n'){
			printc('\r');
		}
		s++;
	}
}

static void early_write(const char *s, unsigned n)
{
	char buf[128];
	while (n) {
		unsigned l = min(n, sizeof(buf)-1);
		memcpy(buf, s, l);
		buf[l] = 0;
		s += l;
		n -= l;
		printasciin(buf);
	}
}

static void early_console_write(struct console *con, const char *s, unsigned n)
{
	early_write(s, n);
}

static struct console early_console_dev = {
	.name =		"earlycon",
	.write =	early_console_write,
	.flags =	CON_PRINTBUFFER | CON_BOOT,
	.index =	-1,
};

static int __init setup_early_printk(char *buf)
{
	early_console = &early_console_dev;
	register_console(&early_console_dev);
	return 0;
}

early_param("earlyprintk", setup_early_printk);
