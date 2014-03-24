#include "server.h"
#include <stdio.h>

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

    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }

    // add the listener to the master set
    FD_SET(listener, &master);

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
      if (FD_ISSET(i, &read_fds)) { // we got one!!
        if (i == listener) {
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
        } else {
           // handle data from a client
            if ((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0) {
             // got error or connection closed by client
              if (nbytes == 0) {
              // connection closed
              printf("selectserver: socket %d hung up\n", i);
              client *c = find_client(cq_header, i);
              if (c->status == 1) {
                chq_header = remove_channel(chq_header, cq_header, i);
              }
               //remove client from client queue
              cq_header = remove_client(cq_header, i);
	      } else {
                perror("recv");
	      }
              close(i); // bye!
              FD_CLR(i, &master); // remove from master set
            } else {
              client *c = find_client(cq_header, i);
              if (c->status == 1) {
                int partner_fd = find_partner(chq_header, i);
                if (send(partner_fd, buf, nbytes, 0) == -1) {
		  perror("send");
                }
              } else if(strcmp(buf, "CHAT") == 0) {
                printf("strcmp done!\n");
                for(j = 0; j <= fdmax; j++) {
                  if (FD_ISSET(j, &master) && j != i && j != listener) {
                    printf("%d\n", j);
                    channel *cha = match_client(cq_header, j, i);
                    chq_header = add_to_channel_queue(chq_header, cha);
                    if (send(i, "IN SESSION", 10, 0) == -1) {
                      perror("send");
                    }
                    if (send(j, "IN SESSION", 10, 0) == -1) {
                      perror("send");
                    }
		                break;
                  }
                }
                if (send(i, "ERROR: Can not find partner!", 28, 0) == -1) {
                  perror("send");
                }
              }
            }
        } // END handle data from client
      } // END got new incoming connection
    } // END looping through file descriptors
    /*
    sin_size = sizeof(their_addr);
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if(new_fd == -1) {
      perror("accpet");
      continue;
    }
    struct hostent *client_hostent = gethostbyaddr(get_in_addr((struct sockaddr *)&their_addr), sizeof(struct in_addr), AF_INET);
    printf("server: got connection from %s\n", client_hostent->h_name);

    //add client to client queue
    cq_header = add_client(cq_header, client_hostent->h_name);
    if(!fork()) { // this is the child process
      close(sockfd);
      if(send(new_fd, "ACKN", 4, 0) == -1)
	perror("send");
      char buf[100];
      int switcher = 1;
      int numbytes;
      int client_fd;
      int partner_fd;
      client *current;
      client *partner;
      while(switcher == 1) {
	if((numbytes = recv(new_fd, buf, 100, 0)) == -1)
	  perror("recv");
	buf[numbytes] = '\0';

	printf("received: %s", buf);
	//buf[numbytes-1] = '\0';
	current = find_client(cq_header, client_hostent->h_name);
	if(current->status == 1) {
	  char *partner_hostname = find_partner(chq_header, current->hostname);
	  partner_fd = connect_to(partner_hostname, CLIENT_PORT);
	  if(strcmp("QUIT", buf) == 0) {
	    if(send(new_fd, "QUIT", 4, 0) == -1)
	      perror("send");
	    chq_header = remove_channel(chq_header, cq_header, current->hostname);
	  } else {
	    if(send(new_fd, buf, strlen(buf), 0) == -1)
	      perror("send");
	  }
	  close(partner_fd);
	  free(partner_hostname);
	  free(current);
	  current = NULL;
	  partner_hostname = NULL;
	  continue;
	}
	if(strcmp(buf, "CHAT") == 0) {
	  partner = find_chat_client(cq_header, client_hostent->h_name);
	  if(partner == NULL) {
	    if(send(new_fd, "ERROR: Can not find partner!", 28, 0) == -1)
	      perror("send");
	  } else {
	    channel *cha = match_client(cq_header, current->hostname, partner->hostname);
	    //check
	    chq_header = add_to_channel_queue(chq_header, cha);
	    // partner_fd = connect_to(partner->hostname, CLIENT_PORT);
	    if(send(new_fd, "IN SESSION", 10, 0) == -1)
	      perror("send");
	    close(partner_fd);
	  }
	  free(partner);
	  partner = NULL;
	} else {
	  if(send(new_fd, "Command unregconized!", 21, 0) == -1)
	      perror("send");
	}
      }
    }*/
  }
}
