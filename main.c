#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "physical_layer.h"
#include "data_link_layer.h"
#include "app_layer.h"

//global variables
int listener;
fd_set master;
int fdmax;
int sockfd;


int main() {
	int i;
	char command[50];
	fd_set read_fds;
	listener = listen_port();
	FD_ZERO(&master);    // clear the master and temp sets
  FD_ZERO(&read_fds);
	FD_SET(0, &master);
	fqueue_init(fqueue, WINDOWSIZE);
	fqueue_init(pqueue, WINDOWSIZE);
	printf("Server is ready, please input command: \n");

	//set up a timer
	signal(SIGALRM. scheduled_handler);
	sigemptyset(&sigs);
	sigaddset(&sigs, SIGALRM);

	struct itimerval tt;
	tt.it_interval.tv_sec = 0;
	tt.it_interval.tv_usec = ALARM_TICK;
	tt.it_value.tv_sec = 0;
	tt.it_value.tv_usec = ALARM_TICK;
	if (setitimer(ITIMER_REAL, &tt, NULL) < 0) {
		perror("sender: setitimer");
		exit(1);
	}

	while(1) {
		read_fds = master;
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
      perror("select");
      exit(4);
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
    		} else {
    			//handle data from a client
    			receiver_handler(i);
    		}
    	}
    }
	}
}

void scheduled_handler() {
	sender_handler();
}