#ifndef _FLY_QUIC_API_H_
#define _FLY_QUIC_API_H_


typedef void (*CB_RECV_PKT)(char*,int);
typedef void (*CB_OnConnectionClosed)(int error_code,int from_remote);
typedef void (*CB_StreamClosed)(int stream_id);


enum APP_CB{
	TYPE_CB_RECV_PKT,
	TYPE_CB_MAX
};

struct FlyQuicSessionConfig 
{
   char is_server;
   char remote_server_id[64];
   char bbr;
   char peer_ip[32];
   short peer_port;
};


#endif
