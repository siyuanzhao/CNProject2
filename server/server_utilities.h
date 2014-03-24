#define HOSTNAME_LENGTH 50
#define NICKNAME_LENGTH 50

typedef struct {
  char hostname[HOSTNAME_LENGTH];
  struct client *next;
  int status; // 0->in wait; 1->in chat
  int sockfd;
  char nickname[NICKNAME_LENGTH];
} client;
#define client_queue client*

typedef struct {
  int client1_fd;
  int client2_fd;
  struct channel *next;
} channel;
#define channel_queue channel*
/*
client* find_chat_client(client_queue header, char* hostname);
client* find_client(client_queue header, char* hostname);
client_queue add_client(client_queue header, char* hostname, int sockfd);
client_queue remove_client(client_queue header, char* hostname);
channel* match_client(client_queue header, char* client1, char* client2);
channel* create_channel(char* client1, char* client2);
channel_queue add_to_channel_queue(channel_queue header, channel* c);
char* find_partner(channel_queue header, char* hostname);
channel_queue remove_channel(channel_queue header, client_queue cq_header, char *hostname);
*/

client* find_chat_client(client_queue header, int sockfd);
client* find_client(client_queue header, int sockfd);
//client_queue add_client(client_queue header, char* hostname, int sockfd);
client_queue add_client(client_queue header, int sockfd);
client_queue remove_client(client_queue header, int sockfd);
channel* match_client(client_queue header, int client1_fd, int client2_fd);
channel* create_channel(int client1_fd, int client2_fd);
channel_queue add_to_channel_queue(channel_queue header, channel* c);
int find_partner(channel_queue header, int sockfd);
channel_queue remove_channel(channel_queue header, client_queue cq_header, int sockfd);
