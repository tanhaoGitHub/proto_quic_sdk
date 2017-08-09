
#include <string>
#include <utility>
#include "../libquic/include/base/logging.h"
#include "../libquic/include/base/files/file_path.h"
#include "../libquic/include/base/at_exit.h"
#include "../libquic/include/base/command_line.h"

#include "fly_quic_session.h"
//using namespace net;

namespace fly {


static net::QuartcSessionInterface::OutgoingStreamParameters kDefaultStreamParam;
static net::QuartcStreamInterface::WriteParameters kDefaultWriteParam;

FlyClock::FlyClock() : now_(net::QuicTime::Zero()) {}

FlyClock::~FlyClock() {}

void FlyClock::AdvanceTime(net::QuicTime::Delta delta) 
{
	now_ = now_ + delta;
}

net::QuicTime FlyClock::Now() const 
{
	return now_;
}

net::QuicTime FlyClock::ApproximateNow() const 
{
	return now_;
}

net::QuicWallTime FlyClock::WallNow() const 
{
	return net::QuicWallTime::FromUNIXSeconds((now_ - net::QuicTime::Zero()).ToSeconds());
}

base::TimeTicks FlyClock::NowInTicks() const 
{
	base::TimeTicks ticks;
	return ticks + base::TimeDelta::FromMicroseconds(
                 (now_ - net::QuicTime::Zero()).ToMicroseconds());
}


/////////////////////////////////////////////////////-->session

void FlyQuicSession::CreateFactory()
{
	net::QuartcFactoryConfig config;
	config.clock = &fly_clock_;
	//factory_.reset(CreateFlyFactory(config));
	factory_ = CreateFlyFactory(config);
}

void FlyQuicSession::CreateSession() 
{
	net::QuicSocketAddress peer_addr(ip,port);
	//session_.reset(factory_.CreateFlyQuicSession(session_config_,
	//												quic_config_,
	//												peer_addr));  
	session_= factory_->CreateFlyQuicSession(session_config_,
													quic_config_,
													peer_addr); 
	
	stream_delegate_.reset(new FlyQuartcStreamDelegate);
	session_delegate_.reset(new FlyQuartcSessionDelegate(stream_delegate_.get()));
	session_->SetDelegate(session_delegate_.get());
}

bool FlyQuicSession::StartQuic()
{
	//if (!session_->StartCryptoHandshake()) {
		//LOG(ERROR) << "Underlying channel is writable but cannot start "
	  	//		 	"the QUIC handshake.";
		//return false;
	//}

	session_->StartCryptoHandshake();
  
	if (!session_->connection()->connected()) {
		LOG(ERROR)<< "QUIC connection should not be closed if underlying "
					"channel is writable.";
		return false;
	}

	LOG(INFO) << "StartQuic ok";

	return true;
}

void FlyQuicSession::CreateStream()
{
	outgoing_stream_=session_->CreateOutgoingStream(kDefaultStreamParam);
}


int  FlyQuicSession::SendPkt(char *buff,size_t len)
{
	if(outgoing_stream_ == nullptr){
		CreateStream();
		if(outgoing_stream_ == nullptr)
			return -1;
		outgoing_stream_->SetDelegate(stream_delegate_.get());
	}

	//EXPECT_TRUE(session_->HasOpenDynamicStreams());
	LOG(INFO) << "tx " << len;
	outgoing_stream_->Write(buff,len,kDefaultWriteParam);

	return 0;
}

void FlyQuicSession::init()
{
	base::AtExitManager at_exit_manager;
	int argc = 1;
  	const char* argv[] = {"fly", nullptr};
  	base::CommandLine::Init(argc, argv);
	logging::LoggingSettings settings;
	settings.logging_dest = logging::LOG_TO_ALL;
	settings.log_file = FILE_PATH_LITERAL("/usr/local/var/log/rnode/quic.log");
	if (!logging::InitLogging(settings)) {
		printf("Error: could not initialize logging. Exiting.\n");
		return -1;
	}
	
	//SetCb();
	
	//LOG_INFO = 0;
	//LOG_WARNING = 1;
	//LOG_ERROR = 2;
	//LOG_FATAL = 3;
	//LOG_NUM_SEVERITIES = 4;
	logging::SetMinLogLevel(0);
}	


///////////////////////////////////////////////////////////



}  // namespace cricket
