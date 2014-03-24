#include "server_utilities.h"
#include <string.h>
#include <stdlib.h>


channel_queue add_to_channel_queue(channel_queue header, channel *c) {
  if(header == NULL) {
    return c;
  }
  c->next = header;
  header = c;
  return header;
}

// add a client into client queue
client_queue add_client(client_queue header, int sockfd) {
  client_queue tmp = (client_queue)malloc(sizeof(client));
  if(tmp == NULL) {
    return header;
  } else {
    //strcpy(tmp->hostname, hostname);
    tmp->status = 0;
    tmp->sockfd = sockfd;
    tmp->next = NULL;
  }

  if(header == NULL) {
    header = tmp;
  } else {
    tmp->next = header;
    header = tmp;
  }
  return header;
}

// find a client with the hostname in client queue
client* find_client(client_queue header, int sockfd) {
  if(header == NULL) {
    return NULL;
  }
  client* p = header;
  while (p != NULL) {
    if(p->sockfd == sockfd) {
      return p;
    }
    p = p->next;
  }
  return NULL;
}

// remove a client from client queue
client_queue remove_client(client_queue header, int sockfd) {
  if(header == NULL) {
    return NULL;
  }

  client_queue cq = header;
  client_queue pre = NULL;
  
  while(cq != NULL) {
    if(cq->sockfd == sockfd) {
      if(pre == NULL) {
        header = cq->next;
        free(cq);
        cq = NULL;
        return header;
      } else {
        pre->next = cq->next;
        free(cq);
        cq = NULL;
        return header;
      }
    } else {
      pre = cq;
      cq = cq->next;
    }
  }
}

channel* create_channel(int client1_fd, int client2_fd) {
  channel* c = (channel*)malloc(sizeof(channel));
  if(c == NULL) {
    return NULL;
  }
  c->client1_fd = client1_fd;
  c->client2_fd = client2_fd;
  return c;
}

channel* match_client(client_queue header, int client1_fd, int client2_fd) {
  client *c1 = find_client(header, client1_fd);
  client *c2 = find_client(header, client2_fd);
  
  if(c1 == NULL || c2 == NULL) {
    printf("Can not find corresponding client!");
    return -1;
  }
  
  channel *cha = create_channel(c1->sockfd, c2->sockfd);
  if(cha == NULL) {
    printf("Error during creating channel!");
    return -1;
  }
  c1->status = 1;
  c2->status = 1;
  return cha;
}

int find_partner(channel_queue header, int sockfd) {
  if(header == NULL) {
    return NULL;
  }
  channel_queue cq = header;
  while(cq != NULL) {
    if(cq->client1_fd == sockfd) {
      return cq->client2_fd;
    } else if(cq->client2_fd == sockfd) {
      return cq->client1_fd;
    }
    cq = cq->next;
  }
  printf("Can not find a partner!");
  return NULL;
}

channel_queue remove_channel(channel_queue header, client_queue cq_header, int sockfd) {
  if(header == NULL) {
    return NULL;
  }
  channel *cq = header;
  channel_queue result = header;
  channel *pre = NULL;
  while(cq != NULL) {
    if(cq->client1_fd == sockfd || cq->client2_fd == sockfd) {
      client *c1 = find_client(cq_header, cq->client1_fd);
      client *c2 = find_client(cq_header, cq->client2_fd);
      c1->status = 0;
      c2->status = 0;
      if(cq == header) {
        result = header->next;
        free(header);
        header = NULL;
      } else {
        pre->next = cq->next;
        free(cq);
        cq = NULL;
      }
      break;
    }
    pre = cq;
    cq = cq->next;
  }
  return result;
}

void print_client_queue(client_queue header) {
  while(header != NULL) {
    printf("sockfd: %d\tstatus:%d\n", header->sockfd, header->status);
    header = header->next;
  }
}
/*
int main() {
  client_queue header = add_client(NULL, 1);
  header = add_client(header, 2);
  header = add_client(header, 3);
  channel *cha = match_client(header, 1, 2);
  channel_queue chq_header = add_to_channel_queue(NULL, cha);
  printf("partner: %d\n", find_partner(chq_header, 1));
  chq_header = remove_channel(chq_header, header, 1);
  print_client_queue(header);

  return 1;
}
*/
