#include <stdio.h>
#include "rocc.h"
#include "spin_lock.h"
#include "ght.h"
#include "ghe.h"
#include "deque.h"


int uart_lock;
char* shadow;

/* This test is used for inter-core communication */

int task_Sender (uint64_t core_id);
int task_Receiver (uint64_t core_id);
/* Core_0 thread */
int main(void)
{
  /* No monitoring is required */


  while(1){

  };
  return 0;
}

/* Core_1 & 2 thread */
int __main(void)
{
  uint64_t Hart_id = 0;
  asm volatile ("csrr %0, mhartid"  : "=r"(Hart_id));
  
  switch (Hart_id){
      case 0x01:
        task_Sender(Hart_id);
      break;

      case 0x02:
        task_Sender(Hart_id);
      break;

      case 0x03:
        task_Sender(Hart_id);
      break;

      case 0x04:
        task_Sender(Hart_id);
      break;

      case 0x05:
        task_Receiver(Hart_id);
      break;

      case 0x06:
        task_Sender(Hart_id);
      break;

      case 0x07:
        task_Sender(Hart_id);
      break;


      default:
      break;
  }
  
  idle();
  return 0;
}

int task_Sender (uint64_t core_id) {
  uint64_t Header = 0x0;  
  uint64_t Payload = 0x0;
  uint64_t index = (core_id << 32);

  for (int i = 0; i < 16 + core_id; i++) {
    while (ghe_agg_status() == GHE_FULL) {
     // Revisit: revise the ghe_full
    }
    Header = Header | index;
    Payload = i;
    ghe_agg_push (Header, Payload);
  }

  while (ghe_agg_status() == GHE_FULL) {
    // Revisit: revise the ghe_full
  }
  ghe_agg_push ((0xFFFFFFFF|index), Payload);
  
  return 0;
}


int task_Receiver (uint64_t core_id) {
  uint64_t Header = 0x0;  
  uint64_t Payload = 0x0;
  uint64_t current_target = 1;
  dequeue  shadow_header[8];
  dequeue  shadow_payload[8];
  
  for (int i = 0; i < 8; i ++)
  {
    initialize(&shadow_header[i]);
    initialize(&shadow_payload[i]);
  }

  lock_acquire(&uart_lock);
  printf("[C Agg]: Initialized!\r\n");
  printf("[C Agg]: Test is now started!\r\n");
  lock_release(&uart_lock);

  while (current_target < 7) {
    while (ghe_status() != GHE_EMPTY){
      ROCC_INSTRUCTION_D (1, Header, 0x0A);
      ROCC_INSTRUCTION_D (1, Payload, 0x0D);
      uint64_t from = (Header>>32) & 0xF;
      uint64_t inst = Header & 0xFFFFFFFF;
    
      if (inst == 0xFFFFFFFF) {
        current_target = current_target + 1;
        lock_acquire(&uart_lock);
        printf("[C Agg]: Core %x core is completed. \r\n", from);
        lock_release(&uart_lock);
      } else {
        enqueueF(&shadow_header[from], Header);
        enqueueF(&shadow_payload[from], Payload);
      }
    }
  } 

 for (int i = 1; i< 8; i++)
 {
   while (empty(&shadow_payload[i]) != 1) {
     u_int64_t comp_header = dequeueR(&shadow_header[i]);
     u_int64_t comp_payload = dequeueR(&shadow_payload[i]);
     lock_acquire(&uart_lock);
     printf("[C Agg]: Recieved a packet. Header: %lx; Payload: %lx \r\n", comp_header, comp_payload);
     lock_release(&uart_lock);
   }
  }


  
  return 0;
}