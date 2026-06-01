// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2022 Michael T. Kloos <michael@michaelkloos.com>
 * Copyright (C) 2026 Jesse Taube <jtaubepe@redhat.com>
 */

#include <linux/bits.h>
#include <linux/export.h>
#include <linux/libgcc.h>
#include <linux/math.h>

#define sign(a) ((a >> (BITS_PER_TYPE(a) - 1)) & 1)

#define MODDIV(a, b, r)					\
do {							\
	typeof(a) j = 0;				\
	typeof(a) i = 0;				\
							\
	while (a >= b) {				\
		if (sign(b)) {				\
			a -= b;				\
			j |= BIT_TYPE(typeof(a), i);	\
			break;				\
		}					\
		b <<= 1;				\
		i++;					\
	}						\
	while (i > 0) {					\
		i--;					\
		b >>= 1;				\
		if (a >= b) {				\
			a -= b;				\
			j |= BIT_TYPE(typeof(a), i);	\
		}					\
	}						\
							\
	r = j;						\
} while (0)

#ifdef CONFIG_GENERIC_LIB_UDIVSI3
unsigned int notrace __udivsi3(unsigned int a, unsigned int b)
{
	unsigned int r;

	if (b == 0)
		return (int)(-1);

	MODDIV(a, b, r);
	return r;
}
EXPORT_SYMBOL(__udivsi3);
#endif

#ifdef CONFIG_GENERIC_LIB_UDIVDI3
unsigned long notrace __udivdi3(unsigned long a, unsigned long b)
{
	unsigned long r;

	if (b == 0)
		return (long)(-1);

	MODDIV(a, b, r);
	return r;
}
EXPORT_SYMBOL(__udivdi3);
#endif

#ifdef CONFIG_GENERIC_LIB_UDIVTI3
unsigned long notrace __udivti3(unsigned long long a, unsigned long long b)
{
	unsigned long r;

	if (b == 0)
		return (long long)(-1);

	MODDIV(a, b, r);
	return r;
}
EXPORT_SYMBOL(__udivti3);
#endif

#ifdef CONFIG_GENERIC_LIB_DIVSI3
int notrace __divsi3(int a, int b)
{
	unsigned int ua = abs(a);
	unsigned int ub = abs(b);
	unsigned int r;

	if (b == 0)
		return (int)(-1);

	MODDIV(ua, ub, r);

	if ((a ^ b) < 0)
		return -r;
	return r;
}
EXPORT_SYMBOL(__divsi3);
#endif

#ifdef CONFIG_GENERIC_LIB_DIVDI3
long notrace __divdi3(long a, long b)
{
	unsigned long ua = abs(a);
	unsigned long ub = abs(b);
	unsigned long r;

	if (b == 0)
		return (long)(-1);

	MODDIV(ua, ub, r);

	if ((a ^ b) < 0)
		return -r;
	return r;
}
EXPORT_SYMBOL(__divdi3);
#endif

#ifdef CONFIG_GENERIC_LIB_DIVTI3
long long notrace __divti3(long long a, long long b)
{
	unsigned long long ua = abs(a);
	unsigned long long ub = abs(b);
	unsigned long long r;

	if (b == 0)
		return (long long)(-1);

	MODDIV(ua, ub, r);

	if ((a ^ b) < 0)
		return -r;
	return r;
}
EXPORT_SYMBOL(__divti3);
#endif

#ifdef CONFIG_GENERIC_LIB_UMODSI3
unsigned int notrace __umodsi3(unsigned int a, unsigned int b)
{
	unsigned int r;

	if (b == 0)
		return a;

	MODDIV(a, b, r);
	return a;
}
EXPORT_SYMBOL(__umodsi3);
#endif

#ifdef CONFIG_GENERIC_LIB_UMODDI3
unsigned long notrace __umoddi3(unsigned long a, unsigned long b)
{
	unsigned long r;

	if (b == 0)
		return a;

	MODDIV(a, b, r);
	return a;
}
EXPORT_SYMBOL(__umoddi3);
#endif

#ifdef CONFIG_GENERIC_LIB_UMODTI3
unsigned long notrace __umodti3(unsigned long long a, unsigned long long b)
{
	unsigned long long r;

	if (b == 0)
		return a;

	MODDIV(a, b, r);
	return a;
}
EXPORT_SYMBOL(__umodti3);
#endif

#ifdef CONFIG_GENERIC_LIB_MODSI3
int notrace __modsi3(int a, int b)
{
	unsigned int ua = abs(a);
	unsigned int ub = abs(b);
	unsigned int r;

	if (b == 0)
		return (int)(-1);

	MODDIV(ua, ub, r);

	if (a < 0)
		return -ua;
	return ua;
}
EXPORT_SYMBOL(__modsi3);
#endif

#ifdef CONFIG_GENERIC_LIB_MODDI3
long notrace __moddi3(long a, long b)
{
	unsigned long ua = abs(a);
	unsigned long ub = abs(b);
	unsigned long r;

	if (b == 0)
		return (long)(-1);

	MODDIV(ua, ub, r);

	if (a < 0)
		return -ua;
	return ua;
}
EXPORT_SYMBOL(__moddi3);
#endif

#ifdef CONFIG_GENERIC_LIB_MODTI3
long long notrace __modti3(long long a, long long b)
{
	unsigned long long ua = abs(a);
	unsigned long long ub = abs(b);
	unsigned long long r;

	if (b == 0)
		return (long long)(-1);

	MODDIV(ua, ub, r);

	if (a < 0)
		return -ua;
	return ua;
}
EXPORT_SYMBOL(__modti3);
#endif
