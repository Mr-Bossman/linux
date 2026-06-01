/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * include/lib/libgcc.h
 */

#ifndef __LIB_LIBGCC_H
#define __LIB_LIBGCC_H

#include <asm/byteorder.h>

typedef int word_type __attribute__ ((mode (__word__)));

#ifdef __BIG_ENDIAN
struct DWstruct {
	int high, low;
};
#elif defined(__LITTLE_ENDIAN)
struct DWstruct {
	int low, high;
};
#else
#error I feel sick.
#endif

typedef union {
	struct DWstruct s;
	long long ll;
} DWunion;

long long notrace __ashldi3(long long u, word_type b);
long long notrace __ashrdi3(long long u, word_type b);
word_type notrace __cmpdi2(long long a, long long b);
long long notrace __lshrdi3(long long u, word_type b);
long long notrace __muldi3(long long u, long long v);
word_type notrace __ucmpdi2(unsigned long long a, unsigned long long b);
unsigned int notrace __udivsi3(unsigned int a, unsigned int b);
unsigned long notrace __udivdi3(unsigned long a, unsigned long b);
unsigned long notrace __udivti3(unsigned long long a, unsigned long long b);
int notrace __divsi3(int a, int b);
long notrace __divdi3(long a, long b);
long long notrace __divti3(long long a, long long b);
unsigned int notrace __umodsi3(unsigned int a, unsigned int b);
unsigned long notrace __umoddi3(unsigned long a, unsigned long b);
unsigned long notrace __umodti3(unsigned long long a, unsigned long long b);
int notrace __modsi3(int a, int b);
long notrace __moddi3(long a, long b);
long long notrace __modti3(long long a, long long b);

#ifdef CONFIG_HAVE_ARCH_LIBGCC_H
#include <asm/libgcc.h>
#endif

#endif /* __ASM_LIBGCC_H */
