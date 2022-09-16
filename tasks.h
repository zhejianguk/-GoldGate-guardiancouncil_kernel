#include <stdint.h>
#include <stdio.h>

extern int uart_lock;
extern char* shadow;

uint64_t task_synthetic ();
uint64_t task_synthetic_malloc (uint64_t base);

int task_hello (int hart_id);

int task_PerfCounter(uint64_t core_id);
int task_Sanitiser(uint64_t core_id);
int task_ShadowStack_S (uint64_t core_id);
int task_ShadowStack_M_Pre (uint64_t core_id);
int task_ShadowStack_M_Agg (uint64_t core_id, uint64_t core_s, uint64_t core_e);