#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLIENT_PORT "9090"
#define PORT "8089" 
int main() {
  //int sockfd = listen_port(port);
  struct sockaddr_storage their_addr; // connector's address information
  socklen_t sin_size;
  int new_fd; // new connection on new_fd
  char s[INET6_ADDRSTRLEN];
  client_queue cq_header = NULL; //client queue
  channel_queue chq_header = NULL; //channel queue
  fd_set master;
  fd_set read_fds;
  int fdmax;
  char command[20];
  char tmp[20];

  int listener;     // listening socket descriptor
  int newfd;        // newly accept()ed socket descriptor
  struct sockaddr_storage remoteaddr; // client address
  socklen_t addrlen;
  int yes=1;        // for setsockopt() SO_REUSEADDR, below
  int i, j, rv;
  char buf[256];    // buffer for client data
  int nbytes;

  char remoteIP[INET6_ADDRSTRLEN];

  struct addrinfo hints, *ai, *p;
  FD_ZERO(&master);    // clear the master and temp sets
  FD_ZERO(&read_fds);

    // get us a socket and bind it
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        
        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    freeaddrinfo(ai); // all done with this
  //wait for admin type 'START'
  while(1) {
    printf("Please input \"START\" to start server!\n");
    gets(command);
    if(strcmp("START", command) == 0)
      break;    
  }
  // listen
  if (listen(listener, 10) == -1) {
    perror("listen");
    exit(3);
  }
    // add the listener to the master set
    FD_SET(listener, &master);
    FD_SET(0, &master);
    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

  printf("server: waiting for connections...\n");

  
  while(1) {

  	read_fds = master;
  	if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
        perror("select");
        exit(4);
    }
    // run through the existing connections looking for data to read
    for(i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) { 
        // handle input
        if (i == 0) {
          gets(command);
          if(command == NULL) {
            printf("command is empty!\n");
          } else if(strcmp("STATS", command) == 0) {
            print_client_queue(cq_header);
            print_channel_queue(chq_header);
          } else if(strcmp("END", command) == 0) {
            empty_client_queue(cq_header);
            empty_channel_queue(chq_header);
            exit(0);
          } else {
            strcpy(tmp, command);
            char *tok = NULL;
            tok = strtok(tmp, " ");
            if(strcmp("BLOCK", tok) == 0) {
              tok = strtok(NULL, " ");
              int fd = atoi(tok);
              printf("%d\n", fd);
              block_client(cq_header, fd);
            } else if(strcmp("UNBLOCK", tok) == 0) {
              tok = strtok(NULL, " ");
              int fd = atoi(tok);
              unblock_client(cq_header, fd);
            } else if(strcmp("THROWOUT", tok) == 0) {
              tok = strtok(NULL, " ");
              int fd = atoi(tok);
              chq_header = remove_channel(chq_header, cq_header, fd);
            } else {
              printf("Unrecognized command.\n");
            }
          }
        } else if (i == listener) {
          // handle new connections
          addrlen = sizeof(remoteaddr);
          newfd = accept(listener,(struct sockaddr *)&remoteaddr,&addrlen);
          if (newfd == -1) {
            perror("accept");
          } else {
            FD_SET(newfd, &master); // add to master set
            if (newfd > fdmax) {    // keep track of the max
              fdmax = newfd;
            }
            printf("selectserver: new connection from %s on socket %d\n",
                inet_ntop(remoteaddr.ss_family,
                get_in_addr((struct sockaddr*)&remoteaddr),
                remoteIP, INET6_ADDRSTRLEN),
                newfd);
            //add client to client queue
            cq_header = add_client(cq_header, newfd);
            if(send(newfd, "ACKN", 4, 0) == -1)
              perror("send");
            }
            print_client_queue(cq_header);
        } else {
           // handle data from a client
            if ((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0) {
             // got error or connection closed by client
              if (nbytes == 0) {
              // connection closed
              printf("selectserver: socket %d hung up\n", i);
              client *c = find_client(cq_header, i);
              if (c->status == 1) {
                int partner_fd = find_partner(chq_header, i);
                if (send(partner_fd, "QUIT", 4, 0) == -1) {
                  perror("send");
                }
                chq_header = remove_channel(chq_header, cq_header, i);
                print_client_queue(cq_header);
              }
               //remove client from client queue
              cq_header = remove_client(cq_header, i);
              print_client_queue(cq_header);
	      } else {
                perror("recv");
	      }
              close(i); // bye!
              FD_CLR(i, &master); // remove from master set
            } else {
              buf[nbytes] = '\0';
              client *c = find_client(cq_header, i);
              // IN CHAT
              if (c->status == 1) {
                int partner_fd = find_partner(chq_header, i);
                if (strcmp("QUIT", buf) == 0) {
                  if (send(partner_fd, "QUIT", 4, 0) == -1) {
                    perror("send");
                  }
                  if (send(i, "QUIT", 4, 0) == -1) {
                    perror("send");
                  }
                  chq_header = remove_channel(chq_header, cq_header, i);
                  print_client_queue(cq_header);
                } else {
                  if (send(partner_fd, buf, nbytes, 0) == -1) {
		                perror("send");
                  }
                  char tmp[70];
                  strcpy(tmp, buf);
                  char *tok = strtok(tmp, " ");
                  if(tok != NULL && strcmp("TRANSFER", tok) == 0) {
                    tok = strtok(NULL, " ");
                    printf("%s\n", tok);
                    char fbuf[1024];
                    int num;
                    //FILE *f;
                    //f = fopen(tok, "w");
                    //int num = 1;
                    //bzero(fbuf, sizeof(fbuf));
                    //while((num = read(i, fbuf, sizeof(fbuf))) > 0) {
                    num = read(i, fbuf, sizeof(fbuf));
                    printf("In transfer.\n");
                    write(partner_fd, fbuf, num);
                      //bzero(fbuf, sizeof(fbuf));
                    //}
                    /*
                    fclose(f);
                    f = fopen(tok, "r");
                    int num = 1;
                    bzero(fbuf, sizeof(fbuf));
                    while(num > 0) {
                      fscanf(f, "%s", fbuf);
                      num = write(partner_fd, fbuf, num);
                      if(num < 0) {
                        break;
                      }
                      bzero(fbuf, sizeof(fbuf));
                    }
                    fclose(f);*/
                  }
                }
              } else if(strcmp(buf, "CHAT") == 0) {
                for(j = 1; j <= fdmax; j++) {
                  if (FD_ISSET(j, &master) && j != i && j != listener) {
                    if(check_client_status(cq_header, j) != 0)
                      continue;
                    channel *cha = match_client(cq_header, j, i);
                    chq_header = add_to_channel_queue(chq_header, cha);
                    if (send(i, "IN SESSION", 10, 0) == -1) {
                      perror("send");
                    }
                    if (send(j, "IN SESSION", 10,