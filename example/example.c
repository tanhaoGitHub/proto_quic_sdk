#include<stdio.h>
#include<stdlib.h>
//#include<sys/epoll.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include <pthread.h>  

//#include<sys/socket.h>
//#include<netinet/in.h>
#include<signal.h>
#include <time.h>
//#include <linux/sockios.h>
//#include <net/if.h>
#include <errno.h>
//#include <sys/resource.h>


#include "../src/fly_quic_api.h"
#include "../src/fly_quic_interface.h"

extern char *optarg;
char *prog;

extern void *fly_quic_create_session(struct FlyQuicSessionConfig *config);
extern int fly_quic_start_quic_ok(void *session);
extern int fly_quic_send_pkt(void *session,char *buff,size_t len);
extern int fly_quic_reg_cb_recv_pkt(void *session,CB_RECV_PKT cb);


static void rx_cb(char *buff,int size)
{
	printf("rx %d\n",size);
}

int main( int argc, char **argv )
{
	int c;
	prog = argv[0];
	struct FlyQuicSessionConfig config;

	void *session;	
	while( ( c = getopt( argc, argv, "Awfp:b:d:e:i:a:j:C:S:B:" ) ) != EOF ){
		switch(c)
		{
			case 'C':
				config.is_server = 0;
				break;
			case 'S':
				config.is_server = 1;
				break;
			case 'd':
				strcpy(config.peer_ip,optarg);
				break;
			case 'p':
				config.peer_port = atoi(optarg);
				break;
			case 'B':
				config.bbr = 1;
				break;
			default:
				break;
		}

	}

	sprintf(config.remote_server_id,"%s:%d",config.peer_ip,config.peer_port);
	session = fly_quic_create_session(&config);
	if(!fly_quic_start_quic_ok(session)){
		printf("start quic failed!");
		exit(-1);
	}

	fly_quic_reg_cb_recv_pkt(session,rx_cb);
	char buff[256] = {0x88};

	while(1){
		fly_quic_send_pkt(session,buff,256);
	}
}

