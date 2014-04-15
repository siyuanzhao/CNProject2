#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include "data_link_layer.h"

#define Packet Frame 
#define PQueue FQueue 

FQueue fqueue;
volatile int base = 0;
volatile int next_seqn = 0;
int expected_seqn = 0;
int new_timer = 0;
int new_sr = 0;
int fd_r;
int fd_w;
int header_size = FRAMESIZE - DATASIZE;
timer_t first_timer;
timer_t second_timer;
timer_t third_timer;
timer_t fourth_timer;
timer_t fifth_timer;
PQueue pqueue;
extern int sockfd;
extern int seqn;
extern int retransmission_mode;
extern debug_info di;

void send_acknowledge(int seqn, int sockfd);
static int make_timer( char *name, timer_t *timerID, int expireMS, int intervalMS );
static void timer_handler( int sig, siginfo_t *si, void *uc );
static int stop_timer(timer_t *timerid);
static int restart_timer(timer_t *timerid, int expireMS, int intervalMS);
static char checksum(char* s);
static void go_back_N();
static void selective_repeat();

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
    if(fqueue.length < fqueue.maxsize) {
      //pause();
      Frame *frame = fqueue_push(&fqueue);
      frame->nbuffer = size;
      frame->seqn = seqn;
      seqn++;
      if(buf != NULL) {
        strncpy(frame->buffer, buf, size);
      }
      frame->checksum = checksum(frame->buffer);
      frame->type = type;    
      printf("Adding data to buffer, length = %d, size = %d\n",fqueue.length, fqueue.maxsize);
    }
}

void sender_handler() {
  //check if there is any unsent frame in queue

  int sent_frames_len = next_seqn - base;
  int counter = 0;
  while((fqueue.length != 0) && (fqueue.length > sent_frames_len)) {
    di.frame_sent_num++;
    int tmp = next_seqn;
    int tmp_seqn;
    if(retransmission_mode == 0)
      tmp_seqn = PMOD(next_seqn, WINDOWSIZE);
    else if(retransmission_mode == 1) 
      tmp_seqn = PMOD(next_seqn, 5);
    Frame *f = fqueue_index(&fqueue, tmp_seqn);
    udt_send(f, FRAMESIZE);
    di.data_amount += (header_size+f->nbuffer);
    next_seqn++;
    printf("After sending, next_seqn = %d, base = %d, length = %d \n", 
      next_seqn, base, fqueue.length);
    fflush(stdout);
    sent_frames_len = next_seqn-base;
    if(retransmission_mode == 1) {
      if(new_sr == 0) {
        //init all timers
        make_timer("first timer", &first_timer, 0, 0);
        make_timer("second timer", &second_timer, 0, 0);
        make_timer("third timer", &third_timer, 0, 0);
        make_timer("fourth timer", &fourth_timer, 0, 0);
        make_timer("fifth timer", &fifth_timer, 0, 0);
        new_sr++;
      }
      switch(tmp_seqn) {
        case 0:
          restart_timer(&first_timer, 100, 100);
          break;
        case 1:
          restart_timer(&second_timer, 100, 100);
          break;
        case 2:
          restart_timer(&third_timer, 100, 100);
          break;
        case 3:
          restart_timer(&fourth_timer, 100, 100);
          break;
        case 4:
          restart_timer(&fifth_timer, 100, 100);
          break;
      }
    } else if(retransmission_mode == 0) {
      if(new_timer == 0) {
        make_timer("first timer", &first_timer, 100, 100);
        new_timer++;
      } else if(tmp == base) {
        restart_timer(&first_timer, 100, 100);
      }
    }
    if(counter > 3) {
      break;
    }
    counter++;
  }
}

void receiver_handler(int sockfd) {
  Frame f;
  int ret = udt_recv(&f, FRAMESIZE);
  struct timeval  tv;
  gettimeofday(&tv, NULL);

  double time1_in_mill = 
         (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
  if(ret < 0) {
    return;
  }
  //check if it is ACK or NAK
  if (f.type == ACK) {
    printf("ACK INFO: seqn: %d base: %d\n", f.seqn, base);
    di.ack_recved_num++;
    //int fseqn = PMOD(f.seqn, WINDOWSIZE);
    while(f.seqn >= base) {
      gettimeofday(&tv, NULL);

      double time2_in_mill = 
         (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
      di.time_required += (time2_in_mill - time1_in_mill);
      if(fqueue.length == 0)
        break;
      if(retransmission_mode == 0) {
        //stop current timter
        //stop_timer(&first_timer);
        //base = PMOD(base+1, WINDOWSIZE);
        base++;
        fqueue_pop(&fqueue);
        printf("After ACK: base: %d, length: %d\n", base, fqueue.length);
        if(base == next_seqn) {
          //stop timer
          stop_timer(&first_timer);
          break;
        } else {
          //start a new timer
          stop_timer(&first_timer);
          restart_timer(&first_timer, 100, 100);
        }
      } else if(retransmission_mode == 1) {
        int tmp_seqn = PMOD(f.seqn, 5);
        switch(tmp_seqn) {
          case 0:
            stop_timer(&first_timer);
            break;
          case 1:
            stop_timer(&second_timer);
            break;
          case 2:
            stop_timer(&third_timer);
            break;
          case 3:
            stop_timer(&fourth_timer);
            break;
          case 4:
            stop_timer(&fifth_timer);
            break;
        }
        base++;
        fqueue_pop(&fqueue);
        printf("After ACK: base: %d, length: %d\n", base, fqueue.length);
      }
    }
  } else {
    //got the expected packet
    if(f.seqn == expected_seqn) {
      gettimeofday(&tv, NULL);

      double time2_in_mill = 
         (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
      di.time_required += (time2_in_mill - time1_in_mill);
      //buffer it till application layer is about to fetch it
      while(pqueue.length == pqueue.maxsize) {//buffer is full, wait till it is available
        //pause();
      }
      if(f.checksum != checksum(f.buffer)) {
        printf("Data is corrupted!\n");
        return;
      }
      Packet *p = fqueue_push(&pqueue);
      p->nbuffer = f.nbuffer;
      if(f.buffer != NULL) {
        strncpy(p->buffer, f.buffer, f.nbuffer);
      }
      p->type = f.type;
      //send back ACK
      send_acknowledge(f.seqn, sockfd);
      expected_seqn++;

    } else if(f.seqn < expected_seqn){
      gettimeofday(&tv, NULL);

      double time2_in_mill = 
         (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
      di.time_required += (time2_in_mill - time1_in_mill);
      di.dup_frame_recved_num++;
      send_acknowledge(expected_seqn-1, sockfd);
    }
  }
}

// Sends an ACK signal back to the sender.
void send_acknowledge(int seqn, int sockfd) {
  di.ack_sent_num++;
  di.data_amount += header_size;
  Frame f;
  f.seqn = seqn;
  f.type = ACK;
  udt_send(&f, FRAMESIZE);
}

//go-back-N retransmission
void retransmission_handler(timer_t *tidp) {
  di.retrans_num++;
  if(retransmission_mode == 0 && *tidp == first_timer) {
    go_back_N();
  } else if(retransmission_mode == 1) {
    selective_repeat(tidp);
  }
}

static int
make_timer(char *name, timer_t *timerID, int expireMS, int intervalMS) {
  struct sigevent te;
  struct itimerspec its;
  struct sigaction sa;
  int sigNo = SIGRTMIN;
  /* Set up signal handler. */
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = timer_handler;
  sigemptyset(&sa.sa_mask);
  if (sigaction(sigNo, &sa, NULL) == -1) {
    fprintf(stderr, "Failed to setup signal handling for %s.\n", name);
    return(-1);
  }
  /* Set and enable alarm */
  te.sigev_notify = SIGEV_SIGNAL;
  te.sigev_signo = sigNo;
  te.sigev_value.sival_ptr = timerID;
  timer_create(CLOCK_REALTIME, &te, timerID);
  its.it_interval.tv_sec = 0;
  its.it_interval.tv_nsec = intervalMS * 1000000;
  its.it_value.tv_sec = 0;
  its.it_value.tv_nsec = expireMS * 1000000;
  timer_settime(*timerID, 0, &its, NULL);
  return(0);
}

static void
timer_handler(int sig, siginfo_t *si, void *uc ) {
  timer_t *tidp;
  tidp = si->si_value.sival_ptr;
  printf("Begin to retransmit!\n");
  fflush(stdout);
  retransmission_handler(tidp);
}

static int stop_timer(timer_t *timerid) {
  struct itimerspec ts;
  ts.it_interval.tv_sec = 0;
  ts.it_interval.tv_nsec = 0;
  ts.it_value.tv_sec = 0;
  ts.it_value.tv_nsec = 0;

  int ret = timer_settime(*timerid, 0, &ts, NULL);
  return ret;
}

static int restart_timer(timer_t *timerid, int expireMS, int intervalMS) {
  struct itimerspec its;
  its.it_interval.tv_sec = 0;
  its.it_interval.tv_nsec = intervalMS * 1000000;
  its.it_value.tv_sec = 0;
  its.it_value.tv_nsec = expireMS * 1000000;

  int ret = timer_settime(*timerid, 0, &its, NULL);
  return ret;
}

void DataLinkRecv() {
  while(pqueue.length != 0) {
    Packet *p = fqueue_pop(&pqueue);
    if(p->type == FILE_STARTER) {
      printf("Ready to creat file!\n");
      fflush(stdout);
      if ((fd_w = open(p->buffer, O_WRONLY | O_CREAT | O_TRUNC,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
        fprintf(stderr, "destination file `%s': ", p->buffer);
        perror("open");
        exit(1);
      }
    } else if(p->type == FILE_DATA) {
      int cnt;
      if((cnt = write(fd_w, p->buffer, p->nbuffer)) < 0) {
        perror("deliver_data: write");
        exit(1);
      }
    } else if (p->type == FILE_END) {
      close(fd_w);
      printf("File transfer completed!\n");
      fflush(stdout);
    } else {
      printf("%s\n", p->buffer);
      fflush(stdout);
    }
  }
}

static char checksum(char* s) {
  signed char sum = -1;
  while (*s != 0) {
    sum += *s;
    s++;
  }
  return sum;
}

static void go_back_N() {
  if(base == next_seqn) {//no need to retransmit
    printf("No need to retransmit!\n");
    return;
  }
  int index;
  for(index = base; index < next_seqn; index++) {
    int i = PMOD(index, WINDOWSIZE);
    Frame *f = fqueue_index(&fqueue, i);
    udt_send(f, FRAMESIZE);
    di.data_amount += (header_size+f->nbuffer);
  }
  restart_timer(&first_timer, 2*100, 2*100);
}

static void selective_repeat(timer_t *tidp) {
  if (*tidp == first_timer ) {
    Frame *f = fqueue_index(&fqueue, 0);
    di.data_amount += (header_size+f->nbuffer);
    udt_send(f, FRAMESIZE);
    restart_timer(&first_timer, 2*100, 2*100);
  } else if(*tidp == second_timer ) {
    Frame *f = fqueue_index(&fqueue, 1);
    di.data_amount += (header_size+f->nbuffer);
    udt_send(f, FRAMESIZE);
    restart_timer(&second_timer, 2*100, 2*100);
  } else if(*tidp == third_timer ) {
    Frame *f = fqueue_index(&fqueue, 2);
    di.data_amount += (header_size+f->nbuffer);
    udt_send(f, FRAMESIZE);
    restart_timer(&third_timer, 2*100, 2*100);
  } else if(*tidp == fourth_timer ) {
    Frame *f = fqueue_index(&fqueue, 3);
    di.data_amount += (header_size+f->nbuffer);
    udt_send(f, FRAMESIZE);
    restart_timer(&fourth_timer, 2*100, 2*100);
  } else if(*tidp == fifth_timer ) {
    Frame *f = fqueue_index(&fqueue, 4);
    di.data_amount += (header_size+f->nbuffer);
    udt_send(f, FRAMESIZE);
    restart_timer(&fifth_timer, 2*100, 2*100);
  }
}