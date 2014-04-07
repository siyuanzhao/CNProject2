#include "app_layer.h"
#include "data_link_layer.h"
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

void run_command(char *command) {
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
		while((cnt = read(fd_s, buf, DATASIZE)) > 0) {
			DataLinkSend(buf, cnt, FILE_DATA);
		}
		if(cnt == 0) {
			DataLinkSend(NULL, 0, FILE_END);
			close(fd_s);
		}
	} else {
		DataLinkSend(command, strlen(command)+1, APPDATA);
	}
}