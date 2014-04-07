#include <stdbool.h>
#define WINDOWSIZE 32	/* window size */
#define BUFFERSIZE 32

#define TIMER_TICK 1000

#define DATASIZE 100 //100 byte
#define FRAMESIZE (sizeof(Frame))
#define ACKSIZE sizeof("ACK")
//type of Frame
#define ACK 1
#define NAK 2
#define FILE_STARTER 3
#define FILE_DATA 4
#define FILE_END 5

/*
  A frame. The header is made of
  - sequence number
  - size of buffer (filled with valid data)
  This is followed by the data.
*/
typedef struct {
  int seqn;
  int nbuffer;
  int type;
  char buffer[DATASIZE];
} Frame;
/*  A very simple circular FIFO queue for frames.
    Push --> [TAIL...HEAD] --> Pop
    We keep the head index, and the length of the queue to compute the tail.
*/
typedef struct {
  int head;
  int length;
  int maxsize;
  Frame* frames;
} FQueue;

int PMOD(int n, int b);
void fqueue_init(FQueue* queue, int windowsize);
void fqueue_destroy(FQueue *queue);
int fqueue_length(FQueue* queue);
Frame* fqueue_tail(FQueue* queue);
Frame* fqueue_head(FQueue* queue);
Frame* fqueue_push(FQueue* queue);
Frame* fqueue_pop(FQueue* queue);
Frame* fqueue_poptail(FQueue* queue);
bool fqueue_empty(FQueue* queue);
/* Applies a function to every frame in the queue */
void fqueue_map(FQueue* queue, void (*fn)(Frame*) );
void fqueue_debug_print(FQueue* queue);
void DataLinkSend(char *buf, int size, int type);
void DataLinkRecv();
