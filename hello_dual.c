#include <stdio.h>
#include "tasks.h"
#include "rocc.h"
#include "ght.h"

int uart_lock;
char* shadow;

int uart_lock = 0;
int *uart_lock_p = &uart_lock;

void lock_acquire(int *lock);
void lock_release (int *lock);


/* Core_0 thread */
int main(void)
{
  unsigned long hart_id;
  asm volatile ("csrr %0, mhartid"  : "=r"(hart_id));

  for (int i = 0; i < 7; i ++)
  {
    task_hello(hart_id);
  }

  while (1){};
  return 0;
}

/* Core_1 thread */
int __main(void)
{
  unsigned long hart_id;
  asm volatile ("csrr %0, mhartid"  : "=r"(hart_id));

  task_hello(hart_id);

  while (1)
  {

  }
  return 0;
}

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