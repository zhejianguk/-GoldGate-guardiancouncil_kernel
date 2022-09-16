#include <stdio.h>
#include <stdlib.h>
#include "rocc.h"
#include "spin_lock.h"
#include "ght.h"
#include "ghe.h"
#include "malloc.h"
#include "tasks.h"

int uart_lock;
char* shadow;

/* Core_0 thread */
int main(void)
{
  
  int *ptr = NULL;
  int ptr_size = 128;
  int sum = 0;
  int sum_temp = 0;

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
  // Data path: MemCaluc
  ght_cfg_filter(0x01, 0x00, 0x03, 0x02); // lb
  ght_cfg_filter(0x01, 0x01, 0x03, 0x02); // lh
  ght_cfg_filter(0x01, 0x02, 0x03, 0x02); // lw
  ght_cfg_filter(0x01, 0x03, 0x03, 0x02); // ld
  ght_cfg_filter(0x01, 0x04, 0x03, 0x02); // lbu
  ght_cfg_filter(0x01, 0x05, 0x03, 0x02); // lhu

  // Insepct store operations 
  // index: 0x02 
  // Func: 0x00; 0x01; 0x02; 0x03
  // Opcode: 0x23
  // Data path: MemCaluc
  ght_cfg_filter(0x02, 0x00, 0x23, 0x03); // sb
  ght_cfg_filter(0x02, 0x01, 0x23, 0x03); // sh
  ght_cfg_filter(0x02, 0x02, 0x23, 0x03); // sw
  ght_cfg_filter(0x02, 0x03, 0x23, 0x03); // sd

  // se: 01, end_id: 0x02, scheduling: rr, start_id: 0x01
  ght_cfg_se (0x01, 0x02, 0x01, 0x01);

  // se: 02, end_id: 0x03, scheduling: rr, start_id: 0x03
  ght_cfg_se (0x02, 0x03, 0x01, 0x03);

  ght_cfg_mapper (0x01, 0b0110);
  ght_cfg_mapper (0x02, 0b0110);



  


  lock_acquire(&uart_lock);
  printf("C0: Test is now started: \r\n");
  lock_release(&uart_lock);
  ght_set_status (0x01); // start monitoring


  //===================== Execution =====================//
  for (int i = 0; i < 170; i++) {
    task_synthetic();
  }

  for (int i = 0; i < 17; i++ ){
    sum_temp = task_synthetic_malloc(i);
    sum = sum + sum_temp;
    printf("");
  }
  


  ptr = (int*) malloc(ptr_size * sizeof(int));
  printf("ptr = %x\r\n", ptr);

  // if memory cannot be allocated
  if(ptr == NULL) {
    printf("C0: Error! memory not allocated.");
    exit(0);
  }

  for (int i = 0; i < ptr_size + 2; i++)
  {
    // Testing detecions of buffer overflow.
    *(ptr + i) = i;
    sum = sum + *(ptr+i);
  }

  free(ptr);



  //=================== Post execution ===================//
  ght_set_status (0x02); // ght: stop
  uint64_t status;
  while (((status = ght_get_status()) < 0x1FFFF) || (ght_get_buffer_status() != GHT_EMPTY)) {

  }

  lock_acquire(&uart_lock);
  printf("All tests are done! Status: %x; Sum: %x -- addr: %x \r\n", status, sum, &sum);
  lock_release(&uart_lock);
  
  // shadow memory
  shadow_free(shadow);
  return 0;
}


/* Core_1 & 2 thread */
int __main(void)
{
  uint64_t Hart_id = 0;
  asm volatile ("csrr %0, mhartid"  : "=r"(Hart_id));
  
  switch (Hart_id){
      case 0x01:
        task_Sanitiser(Hart_id);
      break;

      case 0x02:
        task_Sanitiser(Hart_id);
      break;

      case 0x03:
        task_Sanitiser(Hart_id);
      break;

      case 0x04:
        task_Sanitiser(Hart_id);
      break;

      case 0x05:
        task_Sanitiser(Hart_id);
      break;

      case 0x06:
        task_Sanitiser(Hart_id);
      break;

      case 0x07:
        task_Sanitiser(Hart_id);
      break;

      default:
      break;
  }
  
  idle();
  return 0;
}