#include "fly_quic_api.h"
#include "fly_quic_session.h"

#include "../libquic/include/base/logging.h"

using namespace fly;
using namespace logging;

/*
extern "C" {
void *fly_quic_create_session(struct FlyQuicSessionConfig *config);
int fly_quic_start_quic_ok(void *session);
int fly_quic_send_pkt(void *session,char *buff,size_t len);
int fly_quic_reg_cb_recv_pkt(void *session,CB_RECV_PKT cb);

}
*/

//LOG_INFO = 0;
//LOG_WARNING = 1;
//LOG_ERROR = 2;
//LOG_FATAL = 3;
//LOG_NUM_SEVERITIES = 4;
extern "C" {
void set_log_level(int level) {
  logging::SetMinLogLevel(level);
}


void *fly_quic_create_session(struct FlyQuicSessionConfig *config)
{
	if(!config){
		LOG(ERROR) << "session config is null!";
		return nullptr;
	}
		
	FlyQuicSession *session = new FlyQuicSession(config);
	session->init();
	session->CreateFactory();
	session->CreateSession();
	return session;
}

void fly_quic_release_session(void *psession)
{
	if(!psession){
		LOG(ERROR) << "session  is null!";
		return nullptr;
	}
	FlyQuicSession *session  = 	(FlyQuicSession *)psession;
	session->ReleaseSession();
	delete session;
}

int fly_quic_start_quic_ok(void *psession)
{
	if(!psession){
		LOG(ERROR) << "session is null!";
		return false;
	}
	FlyQuicSession *session  = 	(FlyQuicSession *)psession;
	return session->StartQuic() ? 1 : 0;
}

int fly_quic_send_pkt(void *psession,char *buff,size_t len)
{
	if(!psession || !buff || len==0){
		LOG(ERROR) << "param err!";
		return -1;
	}
	FlyQuicSession *session  = 	(FlyQuicSession *)psession;
	return session->SendPkt(buff,len);
}


int fly_quic_get_outstream_id(void *psession)
{

}

void fly_quic_close_stream(void *psession,int steam_id)
{

}

void fly_quic_cancle_stream(void *psession,int steam_id)
{

}

void fly_quic_reset_stream(void *psession,int steam_id)
{

}

int fly_quic_reg_cb_recv_pkt(void *psession,CB_RECV_PKT cb)
{
	if(!psession || !cb){
		LOG(ERROR) << "session is null or cb is null!";
		return -1;
	}
	FlyQuicSession *session  = 	(FlyQuicSession *)psession;
	session->SetCB_recv_pkt(cb);
	return 0;
}

}

