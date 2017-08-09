#ifndef _FLY_QUIC_INTERFACE_H_
#define _FLY_QUIC_INTERFACE_H_

#include "fly_quic_api.h"
//#include "fly_quic_session.h"

extern "C" {
extern void *fly_quic_create_session(struct FlyQuicSessionConfig *config);
extern int fly_quic_start_quic_ok(void *session);
extern int fly_quic_send_pkt(void *session,char *buff,size_t len);
extern int fly_quic_reg_cb_recv_pkt(void *session,CB_RECV_PKT cb);

}



#endif
