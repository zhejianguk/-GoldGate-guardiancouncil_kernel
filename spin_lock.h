#include <stdint.h>


void lock_acquire(int *lock)
{
	int temp0 = 1;

	__asm__(
		"loop%=: "
		"amoswap.w.aq %1, %1, (%0);"
		"bnez %1,loop%="
		://no output register
		:"r" (lock), "r" (temp0)
		:/*no clobbered registers*/
	);
}

void lock_release (int *lock)
{
	__asm__(
		"amoswap.w.rl x0, x0, (%0);"// Release lock by storing 0.
		://no output
		:"r" (lock)
		://no clobbered register
	);
}

