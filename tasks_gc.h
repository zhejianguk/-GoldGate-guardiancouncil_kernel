#include <stdint.h>
#include <stdio.h>

extern int uart_lock;
extern char* shadow;

int thread_shadowstack_gc (uint64_t core_id);
int thread_shadowstack_agg_gc (uint64_t core_id);
