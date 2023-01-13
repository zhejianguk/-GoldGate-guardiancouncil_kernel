#include <stdio.h>
#include <stdlib.h>
#include "rocc.h"
#include "spin_lock.h"
#include "ght.h"
#include "ghe.h"
#include "malloc.h"
#include "tasks.h"
#include "deque.h"


int task_hello (int hart_id)
{
  lock_acquire(&uart_lock);
  printf("Hello, World! From Hart %d. \n", hart_id);
  lock_release(&uart_lock);

  return 0;
}

uint64_t task_synthetic ()
{
  //===================== Execution =====================//
  __asm__(
          "li   t0,   0x81000000;"         // write pointer
          "li   t1,   0x55555000;"         // data
          "li   t2,   0x81000000;"         // Read pointer
          "j    .loop_store;");

  __asm__(
          ".loop_store:"
          "li   a5,   0x81000FFF;"
          "sw         t1,   (t0);"
          "addi t1,   t1,   1;"             // data + 1
          "addi t0,   t0,   1024;"          // write address + 4
          "blt  t0,   a5,  .loop_store;"
          "li   t0,   0x82000000;"
          "li   t2,   0x81000000;");

   return 0;
}

uint64_t task_synthetic_malloc (uint64_t base)
{
  int *ptr = NULL;
  int ptr_size = 32;
  int sum = 0;

  ptr = (int*) malloc(ptr_size * sizeof(int));
 
  // if memory cannot be allocated
  if(ptr == NULL) {
    printf("Error! memory not allocated. \r\n");
    exit(0);
  }

  for (int i = 0; i < ptr_size; i++)
  {
    *(ptr + i) = base + i;
  }

  for (int i = 0; i < ptr_size; i++)
  {
    sum = sum + *(ptr+i);
  }

  free(ptr);

  return sum;
}



int task_PerfCounter(uint64_t core_id) {
  uint64_t perfc = 0;

  //================== Initialisation ==================//
  ghe_initailised(1);
  
  //===================== Execution =====================// 
  while (ghe_checkght_status() != 0x02){
    uint32_t buffer_depth = ghe_get_bufferdepth();
    while (buffer_depth > 7) {
      ROCC_INSTRUCTION (1, 0x0D);
      ROCC_INSTRUCTION (1, 0x0D);
      ROCC_INSTRUCTION (1, 0x0D);
      ROCC_INSTRUCTION (1, 0x0D);
      ROCC_INSTRUCTION (1, 0x0D);
      ROCC_INSTRUCTION (1, 0x0D);
      ROCC_INSTRUCTION (1, 0x0D);
      ROCC_INSTRUCTION (1, 0x0D);
      perfc = perfc + 8;
      buffer_depth = ghe_get_bufferdepth();
    }

    if (buffer_depth > 0) {
      switch (buffer_depth){
        case 7: 
          ROCC_INSTRUCTION (1, 0x0D);
        case 6: 
          ROCC_INSTRUCTION (1, 0x0D);
        case 5: 
          ROCC_INSTRUCTION (1, 0x0D);
        case 4: 
          ROCC_INSTRUCTION (1, 0x0D);
        case 3: 
          ROCC_INSTRUCTION (1, 0x0D);
        case 2: 
          ROCC_INSTRUCTION (1, 0x0D);
        case 1: 
          ROCC_INSTRUCTION (1, 0x0D);
          perfc = perfc + buffer_depth;
      }
      buffer_depth = 0;
    }
  }

  //=================== Post execution ===================//
  lock_acquire(&uart_lock);
  printf("[C%x PMC]: Completed, PMC = %x! \r\n", core_id, perfc);
  lock_release(&uart_lock);
  ghe_release();

  ghe_initailised(0);  
  return 0;
}


int task_Sanitiser(uint64_t core_id) {
  uint64_t Address = 0x0;
  uint64_t Err_Cnt = 0x0;

  //================== Initialisation ==================//
  while (ghe_checkght_status() == 0x00){
  };

  //===================== Execution =====================// 
  while (ghe_checkght_status() != 0x02){
    while (ghe_status() != GHE_EMPTY){
      uint64_t Header = 0x0;
      uint64_t PC = 0x0;
      uint64_t Inst = 0x0;
      ROCC_INSTRUCTION_D (1, Header, 0x0A);    
      ROCC_INSTRUCTION_D (1, Address, 0x0D);

      asm volatile("fence rw, rw;");

      PC = Header >> 32;
      Inst = Header & 0xFFFFFFFF;

      char bits = shadow[(Address)>>7];
      
      // if(!bits) continue;
      
      // lock_acquire(&uart_lock);
      // printf("[C%x Sani]: Accesses at %x, PC: %x, Inst: %x. \r\n", core_id, Address, PC, Inst);
      // lock_release(&uart_lock);
      if(bits & (1<<((Address&0x70)>>4))) {
        lock_acquire(&uart_lock);
        printf("[C%x Sani]: **Error** illegal accesses at %x, PC: %x, Inst: %x. \r\n", core_id, Address, PC, Inst);
        lock_release(&uart_lock);
        Err_Cnt ++;
        // return -1;
      }
    }


    if ((ghe_status() == GHE_EMPTY) && (ghe_checkght_status() == 0x00)) {
      ghe_complete();
      while((ghe_checkght_status() == 0x00)) {
        // Wait big core to re-start
      }
      ghe_go();
    }
  }


  //=================== Post execution ===================//
  lock_acquire(&uart_lock);
  printf("[C%x Sani]: Completed, %x illegal accesses are detected.\r\n", core_id, Err_Cnt);
  lock_release(&uart_lock);
  ghe_release();      
  
  return 0;
}


int task_ShadowStack_S (uint64_t core_id) {
  uint64_t Header = 0x0;  
  uint64_t Opcode = 0x0;
  uint64_t Func = 0x0;
  uint64_t Rd = 0x0;
  uint64_t RS1 = 0x0;
  uint64_t Payload = 0x0;
  uint64_t PC = 0x0;
  uint64_t Inst = 0x0;
  uint64_t PayloadPush = 0x0;
  uint64_t PayloadPull = 0x0;
  

  //================== Initialisation ==================//
  dequeue  shadow_header;
  dequeue  shadow_payload;
  initialize(&shadow_header);
  initialize(&shadow_payload);

  while (ghe_checkght_status() == 0x00){
  };

  //===================== Execution =====================// 
  while (ghe_checkght_status() != 0x02){
    while (ghe_status() != GHE_EMPTY){
      ROCC_INSTRUCTION_D (1, Header, 0x0A);
      ROCC_INSTRUCTION_D (1, Payload, 0x0D);

      Opcode = Header & 0x7F;
      Func = (Header & 0x7000) >> 12;
      Rd = (Header & 0xF80) >> 7;
      RS1 = (Header & 0xF8000) >> 15;
      PC = Header >> 32;
      Inst = Header & 0xFFFFFFFF;
      
      // Push -- a function is called
      if ((((Opcode == 0x6F) || (Opcode == 0x67)) && (Rd == 0x01)) || // + comprised inst
           ((Opcode == 0x02) && (Func == 0x01) && (Rd != 0x00) && ((RS1 & 0x01) == 0X01))) {
        PayloadPush = Payload;
        if (full(&shadow_payload) == 0) {
          enqueueF(&shadow_header, Header);
          enqueueF(&shadow_payload, PayloadPush);
          lock_acquire(&uart_lock);
          printf("[C%x SS]: <<Pushed>> Expected: %x.                        PC: %x. Inst: %x. \r\n", core_id, PayloadPush, PC, Inst);
          lock_release(&uart_lock);
        }
      }

      // Pull -- a function is returned
      if (((Opcode == 0x67) && (Rd == 0x00)) || // + comprised inst
          ((Opcode == 0X02) && (Func == 0x00 ) && (Rd == 0x01) && ((RS1 & 0x01) == 0X01))) {
        PayloadPull = Payload;
        if (empty(&shadow_payload) == 1) {
          printf("[C%x SS]: ==Empty== Uninteded: %x.                        PC: %x. Inst: %x. \r\n", core_id, PayloadPull, PC, Inst);
        } else {
          u_int64_t comp = dequeueF(&shadow_payload);
          dequeueF(&shadow_header);
          
          if (comp != PayloadPull){
            lock_acquire(&uart_lock);
            printf("[C%x SS]: **Error**  Expected: %x. v.s. Pulled: %x. PC: %x. Inst: %x. \r\n", core_id, comp, PayloadPull, PC, Inst);
            lock_release(&uart_lock);
            // return -1;
          } else {
            lock_acquire(&uart_lock);
            printf("[C%x SS]: --Paried-- Expected: %x. v.s. Pulled: %x. PC: %x. Inst: %x. \r\n", core_id, comp, PayloadPull, PC, Inst);
            lock_release(&uart_lock);
          }
        }
      }
    }
  
    if ((ghe_status() == GHE_EMPTY) && (ghe_checkght_status() == 0x00)) {
      ghe_complete();
      while((ghe_checkght_status() == 0x00)) {
        // Wait big core to re-start
      }
      ghe_go();
    }
  }


  //=================== Post execution ===================//
  if (core_id == 1){
    lock_acquire(&uart_lock);
    printf("[C%x SS]: Completed. No error is detected\r\n", core_id);
    lock_release(&uart_lock);
  }
  ghe_release();

  return 0;
}


int task_ShadowStack_M_Pre (uint64_t core_id) {
  uint64_t Header = 0x0;
  uint64_t Pc = 0x0;  
  uint64_t Opcode = 0x0;
  uint64_t Func = 0x0;
  uint64_t Inst = 0x0;
  uint64_t Rd = 0x0;
  uint64_t RS1 = 0x0;
  uint64_t Payload = 0x0;
  uint64_t PayloadPush = 0x0;
  uint64_t PayloadPull = 0x0;
  uint64_t Header_index = (core_id << 32);
  uint64_t S_Header = 0x00;
  uint64_t S_Payload = 0x00;
  uint64_t cnt = 0x00;

  //================== Initialisation ==================//
  dequeue  shadow_header;
  dequeue  shadow_payload;
  initialize(&shadow_header);
  initialize(&shadow_payload);

  while (ghe_checkght_status() == 0x00){
  };

  //===================== Execution =====================// 
  while (ghe_checkght_status() != 0x02){
    while (ghe_status() != GHE_EMPTY){
      ROCC_INSTRUCTION_D (1, Header, 0x0A);
      ROCC_INSTRUCTION_D (1, Payload, 0x0D);

      Inst = Header & 0xFFFFFFFF;
      Pc = Header >> 32;
      Opcode = Header & 0x7F;
      Func = (Header & 0x7000) >> 12;
      Rd = (Header & 0xF80) >> 7;
      RS1 = (Header & 0xF8000) >> 15;
      
      // Push -- a function is called
      if ((((Opcode == 0x6F) || (Opcode == 0x67)) && (Rd == 0x01)) || // + comprised inst
          ((Opcode == 0x02) && (Func == 0x01) && (Rd != 0x00) && ((RS1 & 0x01) == 0X01))) {
        PayloadPush = Payload;
        if (full(&shadow_payload) == 0) {
          enqueueF(&shadow_header, Inst);
          enqueueF(&shadow_payload, PayloadPush);
        } else {
          lock_acquire(&uart_lock);
          printf("[C%x SS]: **Error** shadow stack is full. \r\n", core_id);
          lock_release(&uart_lock);
        }
      }

      // Pull -- a function is returned
      if (((Opcode == 0x67) && (Rd == 0x00)) || // + comprised inst
          ((Opcode == 0X02) && (Func == 0x00 ) && (Rd == 0x01) && ((RS1 & 0x01) == 0X01))) {
        PayloadPull = Payload;
        if (empty(&shadow_payload) == 1) {
          // Send it to AGG
          while (ghe_agg_status() == GHE_FULL) {
          }
          S_Header = Inst | Header_index;
          S_Payload = Payload;
          ghe_agg_push (S_Header, S_Payload);
        } else {
          u_int64_t comp = dequeueF(&shadow_payload);
          dequeueF(&shadow_header);
          
          if (comp != PayloadPull){
            lock_acquire(&uart_lock);
            printf("[C%x SS]: **Error**  Expected: %x. v.s. Pulled: %x. PC: %x. Inst: %x. \r\n", core_id, comp, PayloadPull, Pc, Inst);
            lock_release(&uart_lock);
          } else {
            // Paried
          }
        }
      }

    // If all push and pull are handled
    if ((ghe_sch_status() == 0x01) && (ghe_status() == GHE_EMPTY)) {
      // Send unpaired pushes 
      while ((empty(&shadow_payload) == 0)) {
        S_Header = dequeueR(&shadow_header);
        S_Header = S_Header | Header_index;
        S_Payload = dequeueR(&shadow_payload);
        while (ghe_agg_status() == GHE_FULL) {
        }
        ghe_agg_push (S_Header, S_Payload);
      }
    
      // Send termination flag
      while (ghe_agg_status() == GHE_FULL) {
      }
      ghe_agg_push ((0xFFFFFFFF|Header_index), 0x0);
      }

      while ((ghe_sch_status() == 0x01) && (ghe_status() == GHE_EMPTY)) {
				// Wait the core to be waked up
			}
    }


  
    if ((ghe_checkght_status() == 0x00) && (ghe_status() == GHE_EMPTY)) {
      ghe_complete();
      while((ghe_checkght_status() == 0x00)) {
        // Wait big core to re-start
      }
      ghe_go();
    }
  }


  //=================== Post execution ===================//
  ghe_release();      
  
  return 0;
}

dequeue  shadow_agg_header;
dequeue  shadow_agg_payload;
dequeue  queues_header[NUM_CORES];
dequeue  queues_payload[NUM_CORES];

void clear_queue(int index)
{
  while (empty(&queues_header[index]) == 0) {
    uint64_t Header_q = dequeueR(&queues_header[index]);
    Header_q = Header_q & 0xFFFFFFFF;
    uint64_t inst = Header_q & 0xFFFFFFFF;
    uint64_t Payload_q = dequeueR(&queues_payload[index]);
    uint64_t Opcode_q = Header_q & 0x7F;
    uint64_t Func_q = (Header_q & 0x7000) >> 12;
    uint64_t Rd_q = (Header_q & 0xF80) >> 7;
    uint64_t RS1_q = (Header_q & 0xF8000) >> 15;
    uint64_t PayloadPush_q = 0x0;
    uint64_t PayloadPull_q = 0x0;
    
             
    if ((((Opcode_q == 0x6F) || (Opcode_q == 0x67)) && (Rd_q == 0x01)) || // + comprised inst
         ((Opcode_q == 0x02) && (Func_q == 0x01) && (Rd_q != 0x00) && ((RS1_q & 0x01) == 0X01))) {
      PayloadPush_q = Payload_q;
      if (full(&shadow_agg_payload) == 0) {
        enqueueF(&shadow_agg_header, Header_q);
        enqueueF(&shadow_agg_payload, PayloadPush_q);
        lock_acquire(&uart_lock);
        printf("[AGG SS]: <<Pushed>> Expected: %x.                        Inst: %x. \r\n", PayloadPush_q, inst);
        lock_release(&uart_lock);
      } else {
        lock_acquire(&uart_lock);
        printf("[AGG SS]: **Error** shadow stack is full. -- Queue index: %x.\r\n", index);
        lock_release(&uart_lock);
        }
      }

    if (((Opcode_q == 0x67) && (Rd_q == 0x00)) || // + comprised inst
        ((Opcode_q == 0X02) && (Func_q == 0x00 ) && (Rd_q == 0x01) && ((RS1_q & 0x01) == 0X01))) {
      PayloadPull_q = Payload_q;
      if (empty(&shadow_agg_payload) == 1) {
        printf("[AGG SS]: **Error** unintended pull. Addr: %x. Inst:%x. -- Queue index: %x. \r\n", PayloadPull_q, Header_q, index);
      } else {
        u_int64_t comp_q = dequeueF(&shadow_agg_payload);
        dequeueF(&shadow_agg_header);

        if (comp_q != PayloadPull_q){
          lock_acquire(&uart_lock);
          printf("[AGG SS]: **Error** %x v.s. %x. Inst: %x. -- Queue index: %x. \r\n", PayloadPull_q, comp_q, Header_q, index);
          lock_release(&uart_lock);
        } else {
          // Successfully paired
          lock_acquire(&uart_lock);
          printf("[AGG SS]: --Paried-- Expected: %x. v.s. Pulled: %x. Inst: %x. \r\n", comp_q, PayloadPull_q, inst);
          lock_release(&uart_lock);
        }
      }
    }
  }
}

uint64_t nxt_target (uint64_t c_current, uint64_t c_start, uint64_t c_end)
{
  uint64_t c_nxt;
  if (c_current == c_end) {
    c_nxt = c_start;
  } else {
    c_nxt = c_current + 1;
  }
  return c_nxt;
}



int task_ShadowStack_M_Agg (uint64_t core_id, uint64_t core_s, uint64_t core_e) {
  uint64_t CurrentTarget = core_s;

  //================== Initialisation ==================//  
  initialize(&shadow_agg_header);
  initialize(&shadow_agg_header);
  for (int i = 0; i < NUM_CORES; i ++)
  {
    initialize(&queues_header[i]);
    initialize(&queues_payload[i]);
  }

  while (ghe_checkght_status() == 0x00){
  };

  //===================== Execution =====================//
  lock_acquire(&uart_lock);
  printf("[AGG SS]: == Current Target: %x. == \r\n", CurrentTarget);
  lock_release(&uart_lock); 

  while (ghe_checkght_status() != 0x02){
    while (ghe_status() != GHE_EMPTY){
      uint64_t Header = 0x0;  
      uint64_t Opcode = 0x0;
      uint64_t Func = 0x0;
      uint64_t Rd = 0x0;
      uint64_t RS1 = 0x0;
      uint64_t Payload = 0x0;
      uint64_t PayloadPush = 0x0;
      uint64_t PayloadPull = 0x0;

      ROCC_INSTRUCTION_D (1, Header, 0x0A);
      ROCC_INSTRUCTION_D (1, Payload, 0x0D);
      uint64_t from = (Header>>32) & 0xF;
      uint64_t inst = Header & 0xFFFFFFFF;

      if (from == CurrentTarget){
        Opcode = inst & 0x7F;
        Func = (Header & 0x7000) >> 12;
        Rd = (inst & 0xF80) >> 7;
        RS1 = (Header & 0xF8000) >> 15;

        // Push -- a function is pushed
        if ((((Opcode == 0x6F) || (Opcode == 0x67)) && (Rd == 0x01)) || // + comprised inst
             ((Opcode == 0x02) && (Func == 0x01) && ((RS1 & 0x01) == 0X01))) {
          PayloadPush = Payload;
          if (full(&shadow_agg_payload) == 0) {
            enqueueF(&shadow_agg_header, Header);
            enqueueF(&shadow_agg_payload, PayloadPush);
            lock_acquire(&uart_lock);
            printf("[AGG SS]: <<Pushed>> Expected: %x.                        Inst: %x. \r\n", PayloadPush, inst);
            lock_release(&uart_lock);
          } else {
            lock_acquire(&uart_lock);
            printf("[AGG SS]: **Error** shadow stack is full. -- Queue index: %x. \r\n", CurrentTarget);
            lock_release(&uart_lock);
          }
        }

        // Pull -- a function is pulled
         if (((Opcode == 0x67) && (Rd == 0x00)) || // + comprised inst
             ((Opcode == 0X02) && (Func == 0x00 ) && (Rd == 0x01) && ((RS1 & 0x01) == 0X01))) {
          PayloadPull = Payload; 
          if (empty(&shadow_agg_payload) == 1) {
            printf("[AGG SS]: **Error** unintended pull. Addr: %x. Inst:%x. -- Queue index: %x. \r\n", PayloadPull, inst, CurrentTarget);
          } else {
            u_int64_t comp = dequeueF(&shadow_agg_payload);
            dequeueF(&shadow_agg_header);
          
            if (comp != PayloadPull){
              lock_acquire(&uart_lock);
              printf("[AGG SS]: **Error** %x v.s. %x. Inst: %x. -- Queue index: %x.\r\n", PayloadPull, comp, inst, CurrentTarget);
              lock_release(&uart_lock);
            } else {
              // Successfully paired
              lock_acquire(&uart_lock);
              printf("[AGG SS]: --Paried-- Expected: %x. v.s. Pulled: %x. Inst: %x. \r\n", comp, PayloadPull, inst);
              lock_release(&uart_lock);
            }
          }
        }
        
        // Clear queue
        if ((Opcode == 0x7F) && (Rd == 0x1F)) {
          clear_queue(CurrentTarget);
          ROCC_INSTRUCTION_S(1, 0x01 << (CurrentTarget-1), 0x21);
          CurrentTarget = nxt_target(CurrentTarget, core_s, core_e);
          lock_acquire(&uart_lock);
          printf("[AGG SS]: == Current Target: %x. == \r\n", CurrentTarget);
          lock_release(&uart_lock); 

          while ((empty(&queues_header[CurrentTarget]) == 0) && 
                 ((queueT(&queues_header[CurrentTarget]) & 0xFFFFFFFF) == 0xFFFFFFFF)) {
            u_int64_t useless;
            useless = dequeueF(&queues_header[CurrentTarget]);
            useless = dequeueF(&queues_payload[CurrentTarget]);
            clear_queue(CurrentTarget);
            ROCC_INSTRUCTION_S(1, 0x01 << (CurrentTarget-1), 0x21);
            CurrentTarget = nxt_target(CurrentTarget, core_s, core_e);
            lock_acquire(&uart_lock);
            printf("[AGG SS]: == Current Target: %x. == \r\n", CurrentTarget);
            lock_release(&uart_lock); 
          }
          clear_queue(CurrentTarget);
        }          
      } else {
        if (full(&queues_header[from]) == 0) {
          enqueueF(&queues_header[from], Header);
					enqueueF(&queues_payload[from], Payload);
        } else {
          lock_acquire(&uart_lock);
    			printf("[AGG SS]: **Error** Asynchronous Full!! from %x, Header = %x, Payload = %x. \r\n", from, Header, Payload);
          lock_release(&uart_lock);
        }
      }
    }

    if ((ghe_checkght_status() == 0x00) && (ghe_status() == GHE_EMPTY)) {
      ghe_complete();
      while((ghe_checkght_status() == 0x00)) {
        // Wait big core to re-start
      }
      ghe_go();
    }
  }


  //=================== Post execution ===================//
  lock_acquire(&uart_lock);
  printf("[AGG SS]: Completed. No error is detected\r\n");
  lock_release(&uart_lock);
  ghe_release();
  
  return 0;
}