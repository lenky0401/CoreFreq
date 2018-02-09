/*
 * CoreFreq
 * Copyright (C) 2015-2018 CYRIL INGENIERIE
 * Licenses: GPL2
 */

#include <sys/ioctl.h>
#include <pthread.h>
#include <math.h>

#include "bitasm.h"
#include "coretypes.h"
#include "corefreq.h"
#include "corefreqm.h"

void Slice_Atomic(void *nil)
{
	unsigned long long xchg = 0x436f757274696174LLU;
	unsigned long long atom = 0x436f726546726571LLU;

	__asm__ __volatile__
	(
		"xchg %0,%1"
		:"=r"((unsigned long long) xchg)
		:"r" (*(volatile long long *) &atom), "r" (xchg)
		:
	);
}

#define CRC32vASM(data, len)						\
({									\
	unsigned int rem = 0;						\
	asm (								\
		"	movl	%[_len], %%r8d"		"\n\t"		\
		"	movq	%[_data], %%r10"	"\n\t"		\
		"	addq	%%r10, %%r8"		"\n\t"		\
		"	movl	$0, %%r12d"		"\n\t"		\
		"	movl	$0x436f7265, %%r9d"	"\n\t"		\
		".LOOP:"				"\n\t"		\
		"	cmpq	%%r8, %%r10"		"\n\t"		\
		"	je	.EXIT"			"\n\t"		\
		"	addq	$1, %%r10"		"\n\t"		\
		"	movzbl	-1(%%r10), %%edx"	"\n\t"		\
		"	xorl	%%edx, %%r12d"		"\n\t"		\
		"	movl	$8, %%edx"		"\n\t"		\
		".INSIDE:"				"\n\t"		\
		"	movl	%%r12d, %%r11d"		"\n\t"		\
		"	shrl	%%r11d"			"\n\t"		\
		"	andl	$1, %%r12d"		"\n\t"		\
		"	cmovne	%%r9d, %%r12d"		"\n\t"		\
		"	xorl	%%r11d, %%r12d"		"\n\t"		\
		"	subl	$1, %%edx"		"\n\t"		\
		"	jne	.INSIDE"		"\n\t"		\
		"	jmp	.LOOP"			"\n\t"		\
		".EXIT:"				"\n\t"		\
		"	movl	%%r12d, %[_rem]"			\
		: [_rem] "+m" (rem)					\
		: [_data] "m" (data),					\
		  [_len] "im" (len)					\
		: "memory", "rdx", "%r8", "%r9", "%r10", "%r11", "%r12" \
	);								\
	rem;								\
})

void Slice_CRC32(void *nil)
{
	unsigned char *data = (unsigned char *) "CYRING_INGENIERIE";
	unsigned int len = 17;

	CRC32vASM(data, len);
}

/*
	Source: 1996 Software graduate thesis
	Conic: a.X² + b.Y² + c.Z² = k
	Variations:
	    1 - l'ellipsoïde
	    2 - l'hyperboloïde à une nappe
	    3 - l'hyperboloïde à deux nappes
	    4 - le cylindre elliptique
	    5 - le cylindre hyperbolique
	    6 - deux plans parallèles
*/
void Slice_Conic(void *pArg)
{
	const double interval = 4.0 * 1024.0;
	const struct {
		double a, b, c, k, step;
	} p[CONIC_VARIATIONS] = {
		{.a=+1.0 , .b=+1.0, .c=-1.0 , .k=    +0.0, .step=8.0 },
		{.a=-3.0 , .b=+3.0, .c=-3.0 , .k=-17000.0, .step=8.0 },
		{.a=+0.01, .b=+0.7, .c=+0.2 , .k= +3000.0, .step=8.0 },
		{.a=+2.0 , .b=+0.0, .c=+1.0 , .k= +5000.0, .step=5.0 },
		{.a=-0.5 , .b=+0.0, .c=+0.75, .k=  +500.0, .step=8.0 },
		{.a=+0.0 , .b=+0.0, .c=+1.0 , .k= +3000.0, .step=28.0}
	};
	double X, Y, Z, Q;
	unsigned long v = (unsigned long) pArg;

	for (Y = -interval; Y <= interval; Y += p[v].step)
	    for (X = -interval; X <= interval; X += p[v].step) {
		Q=(p[v].k - p[v].a * pow(X,2.0) - p[v].b * pow(Y,2.0)) / p[v].c;
		Z = sqrt(Q);

		UNUSED(Z);
	    }
}

RING_SLICE order_list[] = {
	{
		{.cmd=COREFREQ_ORDER_ATOMIC, .arg=COREFREQ_TOGGLE_ON},
		 .func = Slice_Atomic
	},{
		{.cmd=COREFREQ_ORDER_CRC32,  .arg=COREFREQ_TOGGLE_ON},
		 .func = Slice_CRC32
	},{
		{.cmd=COREFREQ_ORDER_CONIC,  .arg=CONIC_ELLIPSOID},
		 .func = Slice_Conic
	},{
		{.cmd=COREFREQ_ORDER_CONIC,  .arg=CONIC_HYPERBOLOID_ONE_SHEET},
		 .func = Slice_Conic
	},{
		{.cmd=COREFREQ_ORDER_CONIC,  .arg=CONIC_HYPERBOLOID_TWO_SHEETS},
		 .func = Slice_Conic
	},{
		{.cmd=COREFREQ_ORDER_CONIC,  .arg=CONIC_ELLIPTICAL_CYLINDER},
		 .func = Slice_Conic
	},{
		{.cmd=COREFREQ_ORDER_CONIC,  .arg=CONIC_HYPERBOLIC_CYLINDER},
		 .func = Slice_Conic
	},{
		{.cmd=COREFREQ_ORDER_CONIC,  .arg=CONIC_TWO_PARALLEL_PLANES},
		 .func = Slice_Conic
	},{
		{},
		 .func = NULL
	}
};