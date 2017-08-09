#ifndef _FLY_QUIC_SESSION_H_
#define _FLY_QUIC_SESSION_H_

#include <memory>
#include <string>
#include "base_sigslot.h"

#include "../libquic/include/net/quic/quartc/quartc_session.h"

#include "../libquic/include/net/quic/core/crypto/crypto_server_config_protobuf.h"
#include "../libquic/include/net/quic/core/quic_simple_buffer_allocator.h"
#include "../libquic/include/net/quic/core/quic_types.h"
#include "../libquic/include/net/quic/quartc/quartc_factory.h"
#include "../libquic/include/net/quic/quartc/quartc_factory_interface.h"
#include "../libquic/include/net/quic/quartc/quartc_packet_writer.h"
#include "../libquic/include/net/quic/quartc/quartc_stream_interface.h"

#include "../libquic/include/net/quic/core/quic_crypto_client_stream.h"
#include "../libquic/include/net/quic/core/quic_crypto_server_stream.h"
#include "../libquic/include/net/quic/core/quic_crypto_stream.h"
#include "../libquic/include/net/quic/core/quic_error_codes.h"
#include "../libquic/include/net/quic/core/quic_session.h"
#include "../libquic/include/net/quic/platform/api/quic_export.h"
#include "../libquic/include/net/quic/quartc/quartc_clock_interface.h"
#include "../libquic/include/net/quic/quartc/quartc_session_interface.h"
#include "../libquic/include/net/quic/quartc/quartc_stream.h"
#include "fly_factory.h"
#include "fly_quic_api.h"

using std::string;

namespace fly {

class FlyClock : public net::QuicClock {
public:
	FlyClock();
	~FlyClock() override;

	// QuicClock implementation:
	net:: QuicTime Now() const override;
	net::QuicTime ApproximateNow() const override;
	net::QuicWallTime WallNow() const override;

	// Advances the current time by |delta|, which may be negative.
	void AdvanceTime(net::QuicTime::Delta delta);

	// Returns the current time in ticks.
	base::TimeTicks NowInTicks() const;

private:
	net::QuicTime now_;
};


// QuartcClock that wraps a MockClock.
//
// This is silly because Quartc wraps it as a QuicClock, and MockClock is
// already a QuicClock.  But we don't have much choice.  We need to pass a
// QuartcClockInterface into the Quartc wrappers.
class FlyQuartcClock : public net::QuartcClockInterface {
public:
	explicit FlyQuartcClock(FlyClock* clock) : clock_(clock) {}
	int64_t NowMicroseconds() override {
		return clock_->WallNow().ToUNIXMicroseconds();
	}

private:
	FlyClock* clock_;
};

// Implemented by the user of the QuartcStreamInterface to receive incoming
 // data and be notified of state changes.
class FlyQuartcStreamDelegate : public net::QuartcStreamInterface::Delegate {
 public:

 // Called when the stream receives the date.
  void OnReceived(net::QuartcStreamInterface* stream,
                  const char* data,
                  size_t size) override 
  {
    last_received_data_ = string(data, size);
    LOG(INFO) << "rx " << size;
    //SignalRecvPkt(data,size);
    if(OnReceived_app)
    	OnReceived_app(data,size);
  }

  void OnClose(net::QuartcStreamInterface* stream) override 
  {
	LOG(INFO) << "stream" << stream->stream_id() << "closed!";
	//SignalStreamClosed(stream->stream_id());
	if(StreamClosed_app)
		StreamClosed_app(stream->stream_id());
  }

  void OnBufferedAmountDecrease(net::QuartcStreamInterface* stream) override {}

  std::string data() { return last_received_data_; }
public: 
  //sigslot::signal2<const char* ,	size_t> SignalRecvPkt;
  //sigslot::signal1<int> SignalStreamClosed;
   CB_RECV_PKT OnReceived_app = nullptr; 
  CB_StreamClosed StreamClosed_app = nullptr;
 private:
   std::string last_received_data_;
};

class FlyQuartcSessionDelegate : public net::QuartcSessionInterface::Delegate {
 public:
  explicit FlyQuartcSessionDelegate(net::QuartcStreamInterface::Delegate* stream_delegate)
      : stream_delegate_(stream_delegate) 
      {

      }
      
  // Called when peers have established forward-secure encryption
  void OnCryptoHandshakeComplete() override {
    	LOG(INFO) << "Crypto handshake complete!";
  }
  
  // Called when connection closes locally, or remotely by peer.
  void OnConnectionClosed(int error_code, bool from_remote) override {
  	LOG(INFO) << "quic connection is closed!";
    connected_ = false;
    //SignalOnConnClosed(error_code,from_remote);
    int is_remote = error_code?1:0;
    if(OnConnectionClosed_app)
    	OnConnectionClosed_app(error_code,is_remote);
  }
  
  // Called when an incoming QUIC stream is created.
  void OnIncomingStream(net::QuartcStreamInterface* quartc_stream) override {
  	LOG(INFO) << "OnIncomingStream!";
    last_incoming_stream_ = quartc_stream;
    last_incoming_stream_->SetDelegate(stream_delegate_);
  }

  net::QuartcStreamInterface* incoming_stream() { return last_incoming_stream_; }

  bool connected() { return connected_; }
public:
  //sigslot::signal2<int, bool> SignalOnConnClosed;
  CB_OnConnectionClosed OnConnectionClosed_app = nullptr;
 private:
  net::QuartcStreamInterface* last_incoming_stream_;
  bool connected_ = true;
  net::QuartcStream::Delegate* stream_delegate_;
};

// Packet writer that does nothing. This is required for QuicConnection but
// isn't used for writing data.
class FlyDummyPacketWriter : public net::QuicPacketWriter {
 public:
  FlyDummyPacketWriter() {}

  // QuicPacketWriter overrides.
  net::WriteResult WritePacket(const char* buffer,
                          size_t buf_len,
                          const net::QuicIpAddress& self_address,
                          const net::QuicSocketAddress& peer_address,
                          net::PerPacketOptions* options) override {
    return net::WriteResult(net::WRITE_STATUS_ERROR, 0);
  }

  bool IsWriteBlockedDataBuffered() const override { return false; }

  bool IsWriteBlocked() const override { return false; };

  void SetWritable() override {}

  net::QuicByteCount GetMaxPacketSize(
      const net::QuicSocketAddress& peer_address) const override {
    return 0;
  }
};


//class FlyQuicSession :public sigslot::has_slots<>{
 class FlyQuicSession {

 public:
 	FlyQuicSession(const struct FlyQuicSessionConfig *config)
	{
		if(config->is_server)
			session_config_.is_server = true;
		else
			session_config_.is_server = false;

		session_config_.unique_remote_server_id.assign(config->remote_server_id);	
		if(config->bbr)
			session_config_.congestion_control =  net::QuartcCongestionControl::kBBR;
		else
			session_config_.congestion_control =  net::QuartcCongestionControl::kDefault;

		
    	ip.FromString(config->peer_ip);
    	port = config->peer_port;
    	//peer_addr_(ip,config->peer_port);
	}
	
  ~FlyQuicSession();
  void CreateSession(); 
  void ReleaseSession(){
		delete outgoing_stream_;
  }
  bool StartQuic(); 
  int  SendPkt(char *buff,size_t len);
  void CreateFactory();
  void init();	
  
public:
  
	//CB_OnConnectionClosed OnConnectionClosed_app = nullptr;
	//void OnConnectionClosed(int error_code, bool from_remote){
	//if(OnConnectionClosed_app)
		//OnConnectionClosed_app(error_code,from_remote?1:0);
	//}
  
  //CB_RECV_PKT OnReceived_app = nullptr; 
  //void OnReceived(char *data,size_t size){
  //		if(OnReceived_app)
  //		OnReceived_app(data,size);
  //}
  
 
  //CB_StreamClosed StreamClosed_app = nullptr;
  //void StreamClosed(int stream_id){
//		if(StreamClosed_app)
//			StreamClosed_app(stream_id);
 // }

 void SetCB_recv_pkt(CB_RECV_PKT cb)
 {
	stream_delegate_->OnReceived_app = cb;
 }

 void SetCB_stream_closed(CB_StreamClosed cb)
 {
	stream_delegate_->StreamClosed_app = cb;
 }

 void SetCB_conn_closed(CB_OnConnectionClosed cb)
 {
	session_delegate_->OnConnectionClosed_app = cb;
 }
 
private:
	void CreateStream();
	
	//void SetCb();
	
	net::QuicConfig quic_config_;
	FlySessionConf session_config_;
	net::QuicIpAddress ip;
	short port;
	//net::QuicSocketAddress peer_addr_(ip,config->peer_port);
	
	net::QuartcStreamInterface *outgoing_stream_ = nullptr;
 	
 	std::unique_ptr<FlyFactory> factory_;
 	//FlyFactory factory_;
 	std::unique_ptr<net::QuartcSession> session_;
 	
	//FlyCB cb_reg_;
	FlyClock clock_;
	FlyQuartcClock fly_clock_{&clock_};

	std::unique_ptr<FlyQuartcStreamDelegate> stream_delegate_;
	std::unique_ptr<FlyQuartcSessionDelegate> session_delegate_;
};

}  // namespace rnode



#endif
