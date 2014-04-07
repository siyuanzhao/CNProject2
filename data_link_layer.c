#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "data_link_layer.h"

#define Packet Frame 
#define PQueue FQueue 

FQueue fqueue;
int base = 0;
int next_seqn = 0;
int expected_seqn = 0;
PQueue pqueue;
extern int sockfd;
void send_acknowledge(int seqn, int sockfd);

/* Positive modulo (n % b); eg. -1 PMOD 32 will return 31 */
int PMOD(int n, int b) {
	 int mod = n % b;
	 return mod >= 0 ? mod : mod + b;
}

void fqueue_init(FQueue* queue, int windowsize) {
  queue->head = 0;
  queue->length = 0;
  queue->maxsize = windowsize;
  queue->frames = malloc(sizeof(Frame) * queue->maxsize);
}
void fqueue_destroy(FQueue* queue) {
  free(queue->frames);
}
int fqueue_length(FQueue* queue) {
  return queue->length;
}
Frame* fqueue_tail(FQueue* queue) {
  int i = (queue->head + queue->length - 1) % queue->maxsize;
  return &queue->frames[i];
}
Frame* fqueue_head(FQueue* queue) {
  assert (queue->head <= queue->maxsize && queue->head >= 0);
  assert (queue->length <= queue->maxsize);
  return &queue->frames[queue->head];
}
Frame* fqueue_push(FQueue* queue) {
  queue->length += 1;
  return fqueue_tail(queue);
}
Frame* fqueue_pop(FQueue* queue) {
  Frame* tmp = fqueue_head(queue);
  queue->head++;
  queue->length--;
  queue->head = PMOD(queue->head, queue->maxsize);
  return tmp;
}
Frame* fqueue_index(FQueue *queue, int index) {
  return &queue->frames[index];
}
Frame* fqueue_poptail(FQueue* queue) {
  Frame* tmp = fqueue_tail(queue);
  assert (fqueue_length(queue) > 0);
  queue->length -= 1;
  return tmp;
}
bool fqueue_empty(FQueue* queue) {
  return queue->length == 0;
}
/* Applies a function to every frame in the queue */
void fqueue_map(FQueue* queue, void (*fn)(Frame*) ) {
  int i = queue->head;
  int last = (queue->head + queue->length - 1) % queue->maxsize;
  if (fqueue_empty(queue)) 
    return;
  while( i != last ) {
    fn(&queue->frames[i]);
    i = PMOD(i+1, queue->maxsize);
  }
  if (fqueue_length(queue) > 1)
    fn(&queue->frames[last]);
}
void fqueue_debug_print(FQueue* queue) {
  if (fqueue_length(queue) > 0) {
    printf("Queue head seq#%d, tail seq#%d, size %d, window size %d\n",
	   fqueue_head(queue)->seqn, fqueue_tail(queue)->seqn,
	   fqueue_length(queue), queue->maxsize);
  } else {
    printf("Empty queue, window size %d\n", queue->maxsize);
  }
}
void DataLinkSend(char *buf, int size, int type) {
  Frame *frame = fqueue_push(&fqueue);
  frame->nbuffer = size;
  int seqn = PMOD(fqueue.head+fqueue.length-1, WINDOWSIZE);
  frame->seqn = seqn;
  strncpy(frame->buffer, buf, size);
  frame->type = type;
}

void sender_handler() {
  //check if there is any unsent frame in queue
  int sent_frames_len = PMOD(next_seqn-base, WINDOWSIZE);
  while(!fqueue_empty(&fqueue) && (fqueue_length(&fqueue) > sent_frames_len)) {
    printf("In sender_handler!\n");
    Frame *f = fqueue_index(&fqueue, next_seqn);
    udt_send(f, FRAMESIZE);
    next_seqn = PMOD(next_seqn+1, WINDOWSIZE);
    sent_frames_len = PMOD(next_seqn-base, WINDOWSIZE);
  }
}

void receiver_handler(int sockfd) {
  Frame f;
  int ret = udt_recv(&f, FRAMESIZE, sockfd);
  if(ret <= 0) {
    return;
  }
  //check if it is ACK or NAK
  if (f.type == ACK) {
    if(f.seqn == base) {
      //stop current timter

      base = PMOD(base+1, WINDOWSIZE);
      fqueue_pop(&fqueue);
      if(base == next_seqn) {
        //stop timer
      } else {
        //start a new timer
      }
    }
  } else {
    //got the expected packet
    if(f.seqn == expected_seqn) {
      //buffer it till application layer is about to fetch it
      while(fqueue_length(&pqueue) == pqueue.maxsize) {//buffer is full, wait till it is available

      }
      Packet *p = fqueue_push(&pqueue);
      p->nbuffer = f.nbuffer;
      strncpy(p->buffer, f.buffer, f.nbuffer);
      printf("%s\t%d\n", p->buffer, f.nbuffer);
      //send back ACK
      send_acknowledge(f.seqn, sockfd);
      expected_seqn = PMOD(expected_seqn+1, WINDOWSIZE);
    }
  }
}

// Sends an ACK signal back to the sender.
void send_acknowledge(int seqn, int sockfd) {
  Frame f;
  f.seqn = seqn;
  f.type = ACK;
  udt_send(&f, FRAMESIZE);
}

//go-back-N retransmission
void retransmission_handler() {
  if(base == next_seqn) //no need to retransfer
    return;
  int index;
  //start a new  timer

  if(next_seqn > base) {
    for(index = base; index < next_seqn; index++) {
      Frame *f = fqueue_index(&fqueue, index);
      udt_send(f, FRAMESIZE);
    }
  } else {
    for(index = base; index < (next_seqn + WINDOWSIZE); index++) {
      int i = PMOD(index, WINDOWSIZE);
      Frame *f = fqueue_index(&fqueue, i);
      udt_send(f, FRAMESIZE);
    }
  }
}
