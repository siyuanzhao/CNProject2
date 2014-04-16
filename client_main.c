#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include "data_link_layer.h"
#include "app_layer.h"

//global variables
fd_set master;
int fdmax;
int seqn = 0;
int sockfd;
int erate = 10;
int listener;
int corrupted = 10;
int retransmission_mode = 0;
debug_info di;
extern FQueue fqueue;
extern FQueue pqueue;
static sigset_t sigs;	/* sigset_t for SIGALRM */

void scheduled_handler();

int main() {
	int i;
	char command[50];
	char tmp[50];
	char action[30];
	fd_set read_fds;
	char *tok = NULL;

	FD_ZERO(&master);    // clear the master and temp sets
  FD_ZERO(&read_fds);
	FD_SET(0, &master);
	fqueue_init(&fqueue, WINDOWSIZE);
	fqueue_init(&pqueue, WINDOWSIZE);
	printf("Client is ready!\n");
	printf("please input frame loss rate (%%): ");
	gets(command);
	erate = atoi(command);
	printf("please input frame corruption rate (%%): ");
	gets(command);
	corrupted = atoi(command);
	printf("please choose retransmission mode(0: go-back-N; 1: selective repeat): \n");
	gets(command);
	di.erate = erate;
	di.corrupted = corrupted;
	di.retrans_mode = retransmission_mode;
	retransmission_mode = atoi(command);

	if(retransmission_mode == 1) {
		fqueue.maxsize = TIMER_NUMBER;
	}

	int status = 0; //0->not connected; 1->connected;
	while(status == 0) {
		printf("Please input command to connect to server:(CONNECT hostname)\n ");
    gets(command);
    strcpy(tmp, command);
    //expect CONNECT command
    tok = strtok(tmp, " ");
    if(tok == NULL) {
			printf("Input the wrong command, please try again!\n");
			continue;
    }
    strcpy(action, tok);
    if(strcmp(action, "CONNECT") == 0) {
    	char hostname[50];
    	tok = strtok(NULL, " ");
    	if(tok) {
	  		strcpy(hostname, tok);
			} else {
	  		printf("Input the wrong command, please try again!\n");
	  		continue;
			}
			if((sockfd = connect_to_server(hostname)) == -1) {
	  		perror("connect");
	  		continue;
			}
			FD_SET(0, &master);
			FD_SET(sockfd, &master);
			fdmax = sockfd;
			break;
    }
	}
	//set up a timer
	signal(SIGALRM, scheduled_handler);
	sigemptyset(&sigs);
	sigaddset(&sigs, SIGALRM);

	struct itimerval tt;
	tt.it_interval.tv_sec = 0;
	tt.it_interval.tv_usec = TIMER_TICK;
	tt.it_value.tv_sec = 0;
	tt.it_value.tv_usec = TIMER_TICK;
	if (setitimer(ITIMER_REAL, &tt, NULL) < 0) {
		perror("sender: setitimer");
		exit(1);
	}

	while(1) {
		read_fds = master;
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
      //perror("select");
      continue;
      //exit(4);
    }
    // run through the existing connections looking for data to read
    for(i = 0; i <= fdmax; i++) {
    	if(FD_ISSET(i, &read_fds)) {
    		if(i==0) {
    			gets(command);
    			run_command(command);
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