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

#define	MTU 1500	/* max transmission unit */
/*
 *	packet format	-- lower layer header + user data
 */
struct lowerpkt {
  int lp_type;			/* packet type */
  char lp_buf[MTU];		/* upper layer data */
};

/* lp_type */
#define	LP_USERDATA	0		/* user data */
#define LP_EOF		1		/* no more user data */

#define	LP_HEADERSIZE	4		/* header size */

/*
 *	packet buffer	-- one per packet
 */
struct pktbuf {
  struct pktbuf *pb_next;
  int pb_stat;			/* status */
  int pb_size;			/* data size */
  int pb_txtime;			/* time when this packet to be sent */
  struct lowerpkt pb_lowerpkt;	/* lower layer packet */
};

#define	PKT_ERR		0x0001		/* packet error */

/*
 *	line buffer	-- emulate communication channel
 */
struct linebuf {
  struct pktbuf *lbuf_head;	/* pktbuf head */
  struct pktbuf *lbuf_tail;	/* pktbuf tail */
  int lbuf_size;			/* buffered data size */
  int lbuf_stat;			/* status */
};

#define LBUF_FULL	0x0001		/* tx channel is full */

/*
 * int udt_send(void *buf, int size)
 *	char *buf;
 *	int size;	size must be less than 1500 (== MTU)
 * return value:
 *	NET_SUCCESS	success
 *	NET_TOOBIG	message size is too big
 *	NET_SYSERR	system call error
 */
int udt_send(void *buf, int size);

/*
 * int udt_recv(void *buf, int size, int timeout)
 *	char *buf;
 *	int size;		buffer size
 *	int timeout;
 *		positive int:	timeout in msec
 *		0:		return immediately even if there is no data
 *		-1:		wait until data is received 
 *
 * return value:
 *	positive int:		received data size
 *	0			there is no data
 *	NET_EOF			EOF
 *	NET_SYSERR		system call error
 */
int udt_recv(void *buf, int size, int timeout);

int listen_port(char* port);
int connect_to_server(char* hostname, char* port);
//get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa);
