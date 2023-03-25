#ifndef DEQUE_H
#define DEQUE_H

#define MAX 128



typedef struct dequeue
{
  uint64_t data[MAX];
  int rear,front;
} dequeue;
 

void initialize(dequeue *p);
int empty(dequeue *p);
int full(dequeue *p);
void enqueueR(dequeue *p,u_int64_t x);
void enqueueF(dequeue *p,u_int64_t x);
u_int64_t dequeueF(dequeue *p);
u_int64_t dequeueR(dequeue *p);
u_int64_t queueT(dequeue *P);

#endif