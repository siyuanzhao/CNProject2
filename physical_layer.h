#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

/*
 *	return value of udt_send() and udt_recv()
 */
#define	NET_SUCCESS	0	/* successful */
#define	NET_EOF		-1	/* EOF */
#define	NET_TOOBIG	-2	/* message is too big */
#define	NET_SYSERR	-3	/* system call error */

#define PORT "8089"
#define	MTU 1500	/* max transmission unit */

void udt_send(void *buf, int size);

void udt_recv(void *buf, int size, int sockfd);
int listen_port();
int connect_to_server(char* hostname);
//get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa);
void create_connection();
