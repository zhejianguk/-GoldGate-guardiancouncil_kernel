#include <stdio.h>
#include <stdlib.h>
#include "deque.h"


void initialize(dequeue *P) {
  P->rear=-1;
  P->front=-1;
}
 
int empty(dequeue *P) {
  if(P->rear==-1)
    return(1);
  return(0);
}
 
int full(dequeue *P) {
  if((P->rear+1)%MAX==P->front)
    return(1);
  return(0);
}
 
void enqueueR(dequeue *P, uint64_t x) {
  if(empty(P)){
    P->rear=0;
    P->front=0;
    P->data[0]=x;
  } else {
    P->rear=(P->rear+1)%MAX;
    P->data[P->rear]=x;
  }
}
 
void enqueueF(dequeue *P,uint64_t x) {
  if(empty(P)) {
    P->rear=0;
    P->front=0;
    P->data[0]=x;
  } else {
    P->front=(P->front-1+MAX)%MAX;
    P->data[P->front]=x;
  }
}
 
uint64_t dequeueF(dequeue *P)  {
  uint64_t x;
  x=P->data[P->front];
  if(P->rear==P->front) {//delete the last element
    initialize(P);
  } else {
    P->front=(P->front+1)%MAX;
  }
  return(x);
}
 
uint64_t dequeueR(dequeue *P) {
  uint64_t x;
  x=P->data[P->rear];
  if(P->rear==P->front)
    initialize(P);
  else
    P->rear=(P->rear-1+MAX)%MAX;
  return(x);
}
 
uint64_t queueT (dequeue *P)  {
  uint64_t x;
  x=P->data[P->front];
  return(x);
}
 