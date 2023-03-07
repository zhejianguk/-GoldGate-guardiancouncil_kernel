#include <stdio.h>
#include <stdlib.h>
#include "rocc.h"
#include "spin_lock.h"
#include "ght.h"
#include "ghe.h"
#include "malloc.h"
#include "tasks_gc.h"
#include "deque.h"



int thread_shadowstack_gc (uint64_t core_id) {
	uint64_t hart_id = (uint64_t) core_id;

	dequeue shadow;

	// GC variables
	uint64_t Header,Payload, PC, Inst, S_Header, S_Payload;
	//================== Initialisation ==================//
	initialize(&shadow);

	ghe_initailised(1);

	//===================== Execution =====================//
	while (ghe_checkght_status() != 0x02){
		while (ghe_status() != GHE_EMPTY){
			ROCC_INSTRUCTION_D (1, Payload, 0x0D);
			uint64_t type = Payload & 0x03;

			if (type == 1) {
				enqueueF(&shadow, Payload);
			} else if (type == 2) {
				if (empty(&shadow) != 1) {
					uint64_t comp = dequeueF(&shadow) + 1;
					if (comp != Payload){
						printf("\r\n [Rocket-C%x-SS]: **Error** Exp:%x v.s. Pul:%x! \r\n", hart_id, comp>>2, Payload>>2);
					}
				} else {
					// Send it to AGG
					/*
					while (ghe_agg_status() == GHE_FULL) {
					}
					ghe_agg_push (hart_id, Payload);
					*/
				}
			}
		}

		// If all push and pull are handled
   		if ((ghe_sch_status() == 0x01) && (ghe_status() == GHE_EMPTY)) {
			// Send unpaired pushes
			while ((empty(&shadow) == 0)) {
				S_Payload = dequeueR(&shadow);
				/*
				while (ghe_agg_status() == GHE_FULL) {
				}
				ghe_agg_push (hart_id, S_Payload);
				*/
			}
		
			// Send termination flag
			while (ghe_agg_status() == GHE_FULL) {
			}
      		ghe_agg_push (hart_id, 0xFFFFFFFF);

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

dequeue queues[NUM_CORES];
dequeue shadow_agg;

void clear_queue(int index)
{
  	while (empty(&queues[index]) == 0) {
		uint64_t Payload_q = dequeueR(&queues[index]);
		uint64_t type_q = Payload_q & 0x03;

		if (type_q == 1) {
			enqueueF(&shadow_agg, Payload_q);
			lock_acquire(&uart_lock);
			printf("[AGG SS]: <<Pushed>> Expected: %x. \r\n", Payload_q>>2);
			lock_release(&uart_lock);
		} else if (type_q == 2) {
			if (empty(&shadow_agg) != 1) {
				uint64_t comp_q = dequeueF(&shadow_agg) + 1;
				if (comp_q != Payload_q){
					lock_acquire(&uart_lock);
					printf("[Rocket-SS-AGG]: **Error** Exp:%x v.s. Pul:%x! \r\n", comp_q>>2, Payload_q>>2);
					lock_release(&uart_lock);
				} else {
					// Successfully paired
          lock_acquire(&uart_lock);
          printf("[AGG SS]: --Paried-- Exp: %x. v.s. Pul: %x. \r\n", comp_q>>2, Payload_q>>2);
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


int thread_shadowstack_agg_gc (uint64_t core_id) {
	uint64_t hart_id = (uint64_t) core_id;
	uint64_t CurrentTarget = 1;

	// GC variables
	uint64_t Header,Payload, PC, Inst;
	//================== Initialisation ==================//
  initialize(&shadow_agg);
  for (int i = 0; i < NUM_CORES; i ++)
  {
    initialize(&queues[i]);
  }


	ghe_initailised(1);
	//===================== Execution =====================//
	if (GC_DEBUG){
			lock_acquire(&uart_lock);
  		printf("[Rocket-SS-AGG]: == Current Target: %x. == \r\n", CurrentTarget);
			lock_release(&uart_lock);
	}

	while (ghe_checkght_status() != 0x02){
		while (ghe_status() != GHE_EMPTY){
			ROCC_INSTRUCTION_D (1, Header, 0x0A);
			ROCC_INSTRUCTION_D (1, Payload, 0x0D);
			uint64_t from = Header& 0xF;

			if (from == CurrentTarget) { // Push
				uint64_t type = Payload & 0x03;
				if (type == 1) {
					enqueueF(&shadow_agg, Payload);
					lock_acquire(&uart_lock);
				printf("[AGG SS]: <<Pushed>> Expected: %x. \r\n", Payload>>2);
				lock_release(&uart_lock);
				} else if (type == 2) {
					if (empty(&shadow_agg) != 1) {// Pull
						uint64_t comp = dequeueF(&shadow_agg) + 1;
						if (comp != Payload){
							lock_acquire(&uart_lock);
							printf("[Rocket-SS-AGG]: **Error** Exp:%x v.s. Pul:%x! \r\n", comp>>2, Payload>>2);
							lock_release(&uart_lock);
						} else {
							// Successfully paired
							lock_acquire(&uart_lock);
							printf("[AGG SS]: --Paried-- Exp: %x. v.s. Pul: %x. \r\n", comp>>2, Payload>>2);
							lock_release(&uart_lock);
						}
					}			
				} else if (type == 3) { // clear queue
					// clear_queue(CurrentTarget);
					ROCC_INSTRUCTION_S(1, 0x01 << (CurrentTarget-1), 0x21); // Restart CurrentTarget for scheduling
					CurrentTarget = nxt_target(CurrentTarget, 1, NUM_CORES-2);
					if (GC_DEBUG){
						lock_acquire(&uart_lock);
						printf("[Rocket-SS-AGG]: == Current Target: %x. == \r\n", CurrentTarget);
						lock_release(&uart_lock);
					}

					while ((empty(&queues[CurrentTarget]) == 0) && 
                 ((queueT(&queues[CurrentTarget]) & 0xFFFFFFFF) == 0xFFFFFFFF)) {
						dequeueF(&queues[CurrentTarget]);
						clear_queue(CurrentTarget);
						ROCC_INSTRUCTION_S(1, 0x01 << (CurrentTarget-1), 0x21); // Restart CurrentTarget for scheduling
						CurrentTarget = nxt_target(CurrentTarget, 1, NUM_CORES-2);
						if (GC_DEBUG){
							lock_acquire(&uart_lock);
							printf("[Rocket-SS-AGG]: == Current Target: %x. == \r\n", CurrentTarget);
							lock_release(&uart_lock);
						}
					}
					clear_queue(CurrentTarget);
				}
			} else {
				enqueueF(&queues[from], Payload);
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
