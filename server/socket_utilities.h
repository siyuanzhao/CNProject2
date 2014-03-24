#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

int listen_port(char* port);
int connect_to(char* hostname, char* port);
//get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa);
