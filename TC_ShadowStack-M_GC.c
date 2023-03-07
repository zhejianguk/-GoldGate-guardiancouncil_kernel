#include <stdio.h>
#include <stdlib.h>
#include "rocc.h"
#include "spin_lock.h"
#include "ght.h"
#include "ghe.h"
#include "tasks.h"
#include "malloc.h"
#include "tasks_gc.h"

int uart_lock;
char* shadow;

/* Core_0 thread */
int main(void)
{

  int sum_temp = 0;
  int sum = 0;

  //================== Initialisation ==================//
  // shadow memory
  shadow = shadow_malloc(32*1024*1024*sizeof(char));
  if(shadow == NULL) {
    printf("C0: Error! memory not allocated.");
    exit(0);
  }

  // Insepct JAL 
  // inst_index: 0x03 
  // Func: 0x00 - 0x07
  // Opcode: 0x6F
  // Data path: ALU + JALR
  ght_cfg_filter(0x03, 0x00, 0x6F, 0x01);
  ght_cfg_filter(0x03, 0x01, 0x6F, 0x01);
  ght_cfg_filter(0x03, 0x02, 0x6F, 0x01);
  ght_cfg_filter(0x03, 0x03, 0x6F, 0x01);
  ght_cfg_filter(0x03, 0x04, 0x6F, 0x01);
  ght_cfg_filter(0x03, 0x05, 0x6F, 0x01);
  ght_cfg_filter(0x03, 0x06, 0x6F, 0x01);
  ght_cfg_filter(0x03, 0x07, 0x6F, 0x01);

  // Insepct JALR 
  // inst_index: 0x03 
  // Func: 0x00
  // Opcode: 0x67
  // Data path: ALU + JALR
  ght_cfg_filter(0x03, 0x00, 0x67, 0x01);

  // Insepct Special RET 
  // inst_index: 0x03
  // Func: 0x00
  // Opcode: 0x02
  // Data path: ALU + JALR
  ght_cfg_filter_rvc(0x03, 0x00, 0x02, 0x01);
  
  // Insepct Special JALR 
  // inst_index: 0x03
  // Func: 0x01
  // Opcode: 0x02
  // Data path: ALU + JALR
  ght_cfg_filter_rvc(0x03, 0x01, 0x02, 0x01);

  ghm_cfg_agg(0x03);
  

  // se: 0x03, end_id: 0x02, scheduling: fp, start_id: 0x01
  ght_cfg_se (0x03, 0x02, 0x03, 0x01);
  ght_cfg_se (0x03, NUM_CORES-2, 0x0f, 0x01); // Reset SE 0x03

  // inst_index: 0x03 se: 0x03
  ght_cfg_mapper (0x03, 0b1000);


  lock_acquire(&uart_lock);
  printf("C0: Test is now started: \r\n");
  lock_release(&uart_lock);
  ght_set_satp_priv();
  ght_set_status (0x01); // ght: start

  //===================== Execution =====================//
  for (int i = 0; i < 170; i++) {
    task_synthetic();
    printf("");
    printf("");
    printf("");
  }

  for (int i = 0; i < 170; i++ ){
    sum_temp = task_synthetic_malloc(i);
    task_synthetic();
    sum = sum + sum_temp;
    printf("");
  }

  

  lock_acquire(&uart_lock);
  printf("Workloads are done!");
  lock_release(&uart_lock);





  //=================== Post execution ===================//
  ght_set_status (0x02);
  uint64_t status;
  while ((status = ght_get_status()) < 0x1FFFF)
  {
    ght_set_status (0x02);
  }

  lock_acquire(&uart_lock);
  printf("All tests are done! Status: %x, sum = %x \r\n", status, sum);
  lock_release(&uart_lock);
  return 0;
}

/* Core_1 & 2 thread */
int __main(void)
{
  uint64_t Hart_id = 0;
  asm volatile ("csrr %0, mhartid"  : "=r"(Hart_id));
  
  switch (Hart_id){
      case 0x01:
        thread_shadowstack_gc(Hart_id);
      break;

      case 0x02:
        thread_shadowstack_gc(Hart_id);
      break;

      case 0x03:
        thread_shadowstack_agg_gc(Hart_id);
      break;

      default:
      break;
  }
  
  idle();
  return 0;
}