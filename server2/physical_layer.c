#include "physical_layer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int listener;
extern int erate;
extern int corrupted;
extern fd_set master;
extern int fdmax;
extern int sockfd;

int rand_lim(int limit);

//start to listen on a specified port; return 1 means OK, return -1 means error
int listen_port() {
  struct addrinfo hints, *servinfo, *p;
  socklen_t sin_size;
  struct sigaction sa;
  int yes = 1;
  int rv;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // use my IP

  if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return -1;
  }

  // loop through all the results and bind to the first we can find
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((listener = socket(p->ai_family, p->ai_socktype,
			 p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }

    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes,
		   sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }

    if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) {
      close(listener);
      perror("server: bind");
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "server: failed to bind\n");
    return -1;
  }

  freeaddrinfo(servinfo); // all done with this structure

  if (listen(listener, 10) == -1) {
    perror("listen");
    exit(1);
  }
  FD_SET(listener, &master);
  fdmax = listener;
  return listener;
}

int create_connection() {
  struct sockaddr_storage remoteaddr; // client address
  socklen_t addrlen;
  char remoteIP[INET6_ADDRSTRLEN];

  addrlen = sizeof(remoteaddr);
  int newfd = accept(listener,(struct sockaddr *)&remoteaddr,&addrlen);
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
      remoteIP, INET6_ADDRSTRLEN), newfd);
  }
  return newfd;
}

int connect_to_server(char* hostname) {
  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(hostname, PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return -1;
  }
  // loop through all the results and connect to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
			 p->ai_protocol)) == -1) {
      perror("client: socket");
      continue;
    }

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("client: connect");
      continue;
    }

    break;
  }
  if (p == NULL) {
    fprintf(stderr, "client: failed to connect\n");
    return -1;
  }
  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
	    s, sizeof(s));
  printf("connecting to %s\n", s);

  freeaddrinfo(servinfo); // all done with this structure
  return sockfd;
}


void *get_in_addr(struct sockaddr *sa) {
  if(sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void udt_send(Frame *buf, int size) {
  if(size > MTU) {
    return;
  }
  int rnd = rand_lim(100); //generate a random number between 1 - 100
  if(rnd < erate) {
    printf("Drop the frame!\n");
    fflush(stdout);
    return;
  }
  rnd = rand_lim(100);
  if(rnd < corrupted) {
    Frame f;
    f.seqn = buf->seqn;
    f.checksum = buf->checksum;
    f.type = buf->type;
    strcpy(f.buffer, "corrupted data!");
    printf("Corrupt the frame!\n");
    fflush(stdout);
    if(send(sockfd, &f, FRAMESIZE, 0) == -1) {
      perror("send");
    return;
  }
  }
  if(send(sockfd, buf, FRAMESIZE, 0) == -1) {
    perror("send");
  }
  //return NET_SUCCESS;
}

int udt_recv(Frame *buf, int size) {
  int nbytes;
  // handle data from a client
  if ((nbytes = recv(sockfd, (void*)buf, size, 0)) <= 0) {
  // got error or connection closed by client
    if (nbytes == 0) {
    // connection closed
      printf("selectserver: socket %d hung up\n", sockfd);
    } else {
      perror("recv");
    }
    close(sockfd); // bye!
    FD_CLR(sockfd, &master); // remove from master set
  }
  return nbytes;
}

int rand_lim(int limit) {
/* return a random number between 0 and limit inclusive.
 */
  int divisor = RAND_MAX/(limit+1);
  int retval;

  do { 
    retval = rand() / divisor;
  } while (retval > limit);
  return retval;
}