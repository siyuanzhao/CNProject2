#include "client.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAXDATASIZE 200
#define COMMAND_LENGTH 70
#define PORT "9090"

int main() {
  printf("Client application starts!\n");

  char command[70];
  char tmp[70];
  char action[10];
  char *tok = NULL;
  char buf[MAXDATASIZE];
  int numbytes;
  int status = 0; //0->not connected; 1->connected; 2->in chat
  int sockfd, fdmax, i;
  fd_set master;
  fd_set read_fds;
  int file_transfer = 0;
  // while(1) {

    while(status == 0) {    
      printf("Please input command:(For example, CONNECT hostname port)\n ");
      gets(command);

      //check if input is legal
      if(command == NULL) {
	printf("Input the wrong command, please try again!\n");
	continue;
      }
      strcpy(tmp, command);
      //expect CONNECT command
      tok = strtok(tmp, " ");
      if(tok == NULL) {
	printf("Input the wrong command, please try again!\n");
	continue;
      }
      strcpy(action, tok);
      if(strcmp(action, "CONNECT") == 0) {
	if(status != 0) {
	  printf("You have already connected to the server! Try command CHAT\n");
	  continue;
	}
	char hostname[50];
	char port[10];
	tok = strtok(NULL, " ");
	if(tok) {
	  strcpy(hostname, tok);
	} else {
	  printf("Input the wrong command, please try again!\n");
	  continue;
	}
	tok = strtok(NULL, " ");
	if(tok) {
	  strcpy(port, tok);
	} else {
	  printf("Input the wrong command, please try again!\n");
	  continue;
	}
	printf("%s\t%s\n", hostname, port);
	if((sockfd = connect_to(hostname, port)) == -1) {
	  perror("connect");
	  continue;
	}
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	FD_SET(0, &master);
	FD_SET(sockfd, &master);
	fdmax = sockfd;
	break;
      }
    }
    while(1) {
      read_fds = master;
      if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
	perror("select");
	continue;
      } 
      for(i=0; i <= fdmax; i++)
	if(FD_ISSET(i, &read_fds)) {
	  if(i == 0) {
	    gets(command);
	    if(send(sockfd, command, strlen(command), 0) == -1) {
	      perror("send");
	    }

    	    strcpy(tmp, command);
	    char *tok = strtok(tmp, " ");
	    if(tok != NULL && strcmp("TRANSFER", tok) == 0) {
	      tok = strtok(NULL, " ");
	      char fbuf[1024];
	      FILE *f;
	      f = fopen(tok, "r");
	      long file_size;
	      fseek(f, 0, SEEK_END);
	      file_size = ftell(f);
	      rewind(f);
	      int result = fread(fbuf, 1, file_size, f);
	      write(sockfd, fbuf, result);
	      //int num = 1;
	      /*
	      while(fscanf(f, "%s\n", fbuf) != EOF) {
		//read(f, fbuf, sizeof(fbuf));
		if((num = write(sockfd, fbuf, sizeof(fbuf))) < 0)
		  break;
		bzero(fbuf, sizeof(fbuf));
		}*/
	      fclose(f);
	      printf("File was sent successfully.");
	    }
	  } else if(i == sockfd) {
	    if((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	      perror("recv");
	      continue;
	    }
	    if(numbytes == 0) {
	      printf("Server stopped. Application quits!\n");
	      exit(0);
	    }
	    buf[numbytes] = '\0';
	    printf("%s\n", buf);
	    fflush(stdout);
	    char tmp[70];
	    strcpy(tmp, buf);
	    char *tok = strtok(tmp, " ");
	    if(tok != NULL && strcmp("TRANSFER", tok) == 0) {
	      char fbuf[1024];
	      tok = strtok(NULL, " ");
	      FILE *f;
	      bzero(fbuf, sizeof(fbuf));
	      int num = read(sockfd, fbuf, 1024);
	      f = fopen(tok, "w");
	      fwrite(fbuf, sizeof(char), num, f);
	      printf("The file was received successfully.");
	      fclose(f);
	    }
	  } //else if(i == sockfd)
	}// if(FD_ISSET)
    }//while(1)
    // }   
}
