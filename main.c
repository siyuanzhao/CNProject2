#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include "app_layer.h"
#include "data_link_layer.h"

//global variables
int listener;
fd_set master;
int fdmax;
int sockfd;
int erate = 10;
int corrupted = 10;
int seqn = 0;
int retransmission_mode = 0; // 0=>go-back-N; 1=>selective repeat
debug_info di;
extern FQueue fqueue;
extern FQueue pqueue;
static sigset_t sigs;	/* sigset_t for SIGALRM */
extern int base;
extern int next_seqn;
extern int expected_seqn;
extern int new_timer;
extern int new_sr;

void scheduled_handler();

int main() {
	int i;
	int j;
	char command[50];
	fd_set read_fds;
	FD_ZERO(&master);    // clear the master and temp sets
  FD_ZERO(&read_fds);
	FD_SET(0, &master);
	fqueue_init(&fqueue, WINDOWSIZE);
	fqueue_init(&pqueue, WINDOWSIZE);
	listener = listen_port();
	printf("Server is ready, please input frame loss rate (%%): ");
	gets(command);
	erate = atoi(command);
	printf("please input frame corruption rate (%%): ");
	gets(command);
	corrupted = atoi(command);
	printf("please choose retransmission mode(0: go-back-N; 1: selective repeat): \n");
	gets(command);
	retransmission_mode = atoi(command);
	/*
	if(retransmission_mode == 1) {
		fqueue.maxsize = 5;
	}*/

	//set up a timer
	signal(SIGALRM, scheduled_handler);
	sigemptyset(&sigs);
	sigaddset(&sigs, SIGALRM);

	struct itimerval tt;
	tt.it_interval.tv_sec = 0;
	tt.it_interval.tv_usec = 2*TIMER_TICK;
	tt.it_value.tv_sec = 0;
	tt.it_value.tv_usec = 2*TIMER_TICK;

	if (setitimer(ITIMER_REAL, &tt, NULL) < 0) {
		perror("sender: setitimer");
		exit(1);
	}

	while(1) {
		read_fds = master;
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
      //perror("select");
      continue;
    }
    // run through the existing connections looking for data to read
    for(i = 0; i <= fdmax; i++) {
    	if(FD_ISSET(i, &read_fds)) {
    		if(i==0) {
    			gets(command);
    			run_command(command);
    		} else if(i == listener) {
    			//handle new connection
    			sockfd = create_connection();
    			//initialize global variables
    			seqn = 0;
    			pqueue.head = 0;
    			pqueue.length = 0;
    			base = 0;
    			next_seqn = 0;
    			expected_seqn = 0;
    			new_timer = 0;
    			new_sr = 0;
    		} else {
    			//handle data from a client
    			receiver_handler(i);
					DataLinkRecv();
    		}
    	}
    }
	}
}

void scheduled_handler() {
	sender_handler();
}