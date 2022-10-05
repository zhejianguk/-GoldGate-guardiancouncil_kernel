#include <stdio.h>
#include <stdlib.h>
#include "rocc.h"
#include "spin_lock.h"
#include "ght.h"
#include "ghe.h"
#include "tasks.h"
#include "malloc.h"

int uart_lock;
char* shadow;

/* Core_0 thread */
int main(void)
{
  uint64_t satp = 0;
  uint64_t priv = 0;

  int sum_temp = 0;
  int sum = 0;

  //================== Initialisation ==================//
  // shadow memory
  shadow = shadow_malloc(32*1024*1024*sizeof(char));
  if(shadow == NULL) {
    printf("C0: Error! memory not allocated.");
    exit(0);
  }

  // Insepct load operations 
  // index: 0x01 
  // Func: 0x00; 0x01; 0x02; 0x03; 0x04; 0x05
  // Opcode: 0x03
  // Data path: ALU two cycles before
  ght_cfg_filter(0x01, 0x00, 0x03, 0x02); // lb
  ght_cfg_filter(0x01, 0x01, 0x03, 0x02); // lh
  ght_cfg_filter(0x01, 0x02, 0x03, 0x02); // lw
  ght_cfg_filter(0x01, 0x03, 0x03, 0x02); // ld
  ght_cfg_filter(0x01, 0x04, 0x03, 0x02); // lbu
  ght_cfg_filter(0x01, 0x05, 0x03, 0x02); // lhu
  
  // Insepct store operations 
  // index: 0x01 
  // Func: 0x00; 0x01; 0x02; 0x03
  // Opcode: 0x23
  // Data path: ALU two cycles before
  ght_cfg_filter(0x02, 0x00, 0x23, 0x03); // sb
  ght_cfg_filter(0x02, 0x01, 0x23, 0x03); // sh
  ght_cfg_filter(0x02, 0x02, 0x23, 0x03); // sw
  ght_cfg_filter(0x02, 0x03, 0x23, 0x03); // sd

  // Insepct JAL 
  // inst_index: 0x03 
  // Func: 0x00 - 0x07
  // Opcode: 0x6F
  // Data path: ALU + JALR
  ght_cfg_filter(0x03, 0x00, 0x6F, 0x04);
  ght_cfg_filter(0x03, 0x01, 0x6F, 0x04);
  ght_cfg_filter(0x03, 0x02, 0x6F, 0x04);
  ght_cfg_filter(0x03, 0x03, 0x6F, 0x04);
  ght_cfg_filter(0x03, 0x04, 0x6F, 0x04);
  ght_cfg_filter(0x03, 0x05, 0x6F, 0x04);
  ght_cfg_filter(0x03, 0x06, 0x6F, 0x04);
  ght_cfg_filter(0x03, 0x07, 0x6F, 0x04);

  // Insepct JALR 
  // inst_index: 0x03 
  // Func: 0x00
  // Opcode: 0x67
  // Data path: ALU + JALR
  ght_cfg_filter(0x03, 0x00, 0x67, 0x04);

  // Insepct Compressed JALR 
  // inst_index: 0x03
  // Func: 0x00
  // Opcode: 0x02
  // Data path: ALU + JALR
  ght_cfg_filter(0x03, 0x00, 0x02, 0x04);
  
  // Insepct Compressed JAL
  // inst_index: 0x03
  // Func: 0x01
  // Opcode: 0x02
  // Data path: ALU + JALR
  ght_cfg_filter(0x03, 0x01, 0x02, 0x04);

  // se: 00, end_id: 0x01, scheduling: rr, start_id: 0x01
  ght_cfg_se (0x00, 0x01, 0x01, 0x01);
  // se: 01, end_id: 0x03, scheduling: rr, start_id: 0x02
  ght_cfg_se (0x01, 0x03, 0x01, 0x02);
  // se: 03, end_id: 0x06, scheduling: fp, start_id: 0x05
  ght_cfg_se (0x03, 0x06, 0x03, 0x05);

  ght_cfg_mapper (0x01, 0b0011);
  ght_cfg_mapper (0x02, 0b0010);
  ght_cfg_mapper (0x03, 0b1000);

  ghm_cfg_agg(0x04);
  
  lock_acquire(&uart_lock);
  printf("C0: Test is now started: \r\n");
  lock_release(&uart_lock);
  ght_set_status (0x01); // ght: start

  satp = ght_get_satp();
  priv = ght_get_priv();
  lock_acquire(&uart_lock);
  printf("C0: PTBR = %x; PRIV= %x \r\n", satp, priv);
  lock_release(&uart_lock);
  //===================== Execution =====================//
  for (int i = 0; i < 170; i++) {
    task_synthetic();
  }

  for (int i = 0; i < 170; i++ ){
    sum_temp = task_synthetic_malloc(i);
    sum = sum + sum_temp;
    printf("");
  }



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
        task_PerfCounter(Hart_id);
      break;

      case 0x02:
        task_Sanitiser(Hart_id);
      break;

      case 0x03:
        task_Sanitiser(Hart_id);
      break;

      case 0x04:
        task_ShadowStack_M_Agg(Hart_id, 0x05, 0x06);
      break;

      case 0x05:
        task_ShadowStack_M_Pre(Hart_id);
      break;

      case 0x06:
        task_ShadowStack_M_Pre(Hart_id);
      break;

      case 0x07:
        task_ShadowStack_M_Pre(Hart_id);
      break;

      default:
      break;
  }
  
  idle();
  return 0;
}
