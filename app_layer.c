#include "app_layer.h"
#include "data_link_layer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

extern int erate;
extern int corrupted;
extern int retransmission_mode;
extern FQueue fqueue;
extern FQueue pqueue;
extern debug_info di;

void run_command(char *command) {
	char value[50];
	if(strcmp("transfer", command) == 0) {
		printf("Please input the file name: \n");
		char filename[50];
		gets(filename);
		int fd_s;	/* file for tx */
		while((fd_s = open(filename, O_RDONLY)) < 0) {
			perror("open");
			printf("Please input the file name: \n");
			gets(filename);
		}
		DataLinkSend(filename, strlen(filename)+1, FILE_STARTER);

		int cnt;

		char buf[DATASIZE];
		int quit = 0;
		while((cnt = read(fd_s, buf, DATASIZE)) > 0) {
			DataLinkSend(buf, cnt, FILE_DATA);
		}
		if(cnt == 0) {
			DataLinkSend(NULL, 0, FILE_END);
			close(fd_s);
		}
	} else if(strcmp("loss rate", command) == 0) {
		printf("Please input the new value: \n");
		gets(value);
		erate = atoi(value);
	} else if(strcmp("corruption rate", command) == 0) {
		printf("Please input the new value: \n");
		gets(value);
		corrupted = atoi(value);
	} else if(strcmp("retrans mode", command) == 0) {
		printf("please choose retransmission mode(0: go-back-N; 1: selective repeat): \n");
		gets(value);
		retransmission_mode = atoi(value);
		if(retransmission_mode == 1) {
			fqueue.maxsize = 5;
		} else if(retransmission_mode == 0) {
			fqueue.maxsize = WINDOWSIZE;
		}
	} else if(strcmp("print", command) == 0) {
		printf("retransmission mode: %d\nerate %%: %d\ncorruption %%: %d\nframe sent num: %d\n\
			retrans_num: %d\nack_sent_num: %d\nack_recved_num: %d\ndata_amount: %d\n\
			dup_frame_recved_num: %d\ntime_required: %f\n",di.retrans_mode, di.erate, di.corrupted,\
			 di.frame_sent_num, di.retrans_num,di.ack_sent_num, di.ack_recved_num, di.data_amount, \
			 di.dup_frame_recved_num, di.time_required);
	} else {
		DataLinkSend(command, strlen(command)+1, APPDATA);
	}
}