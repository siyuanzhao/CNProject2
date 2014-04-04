#include <stdlib.h>
#include <assert.h>
#include "data_link_layer.h"

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
  assert (queue->head <= queue->maxsize && queue->head >= 0);
  assert (queue->length <= queue->maxsize);
  return &queue->frames[i];
}
Frame* fqueue_head(FQueue* queue) {
  assert (queue->head <= queue->maxsize && queue->head >= 0);
  assert (queue->length <= queue->maxsize);
  return &queue->frames[queue->head];
}
Frame* fqueue_push(FQueue* queue) {
  assert (fqueue_length(queue) < queue->maxsize);
  queue->length += 1;
  return fqueue_tail(queue);
}
Frame* fqueue_pop(FQueue* queue) {
  Frame* tmp = fqueue_head(queue);
  assert (fqueue_length(queue) > 0);
  queue->head++;
  queue->length--;
  queue->head = PMOD(queue->head, queue->maxsize);
  return tmp;
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
