#include "../libquic/include/net/quic/quartc/quartc_factory.h"
#include "../libquic/include/net/quic/core/crypto/quic_random.h"
#include "../libquic/include/net/quic/platform/api/quic_socket_address.h"
#include "../libquic/include/net/quic/quartc/quartc_session.h"
#include "../libquic/include/net/quic/core/quic_types.h"
#include "fly_factory.h"


namespace fly {

namespace {
	static net::QuicConnectionId connection_id = 0;

// Implements the QuicAlarm with QuartcTaskRunnerInterface for the Quartc
//  users other than Chromium. For example, WebRTC will create QuartcAlarm with
// a QuartcTaskRunner implemented by WebRTC.
class FlyAlarm : public net::QuicAlarm, public net::QuartcTaskRunnerInterface::Task {
 public:
  FlyAlarm(const net::QuicClock* clock,
              net::QuartcTaskRunnerInterface* task_runner,
              net::QuicArenaScopedPtr<net::QuicAlarm::Delegate> delegate)
      : QuicAlarm(std::move(delegate)),
        clock_(clock),
        task_runner_(task_runner) {}

  ~FlyAlarm() override {
    // Cancel the scheduled task before getting deleted.
    CancelImpl();
  }

  // QuicAlarm overrides.
  void SetImpl() override {
    DCHECK(deadline().IsInitialized());
    // Cancel it if already set.
    CancelImpl();

    int64_t delay_ms = (deadline() - (clock_->Now())).ToMilliseconds();
    if (delay_ms < 0) {
      delay_ms = 0;
    }

    DCHECK(task_runner_);
    DCHECK(!scheduled_task_);
    scheduled_task_ = task_runner_->Schedule(this, delay_ms);
  }

  void CancelImpl() override {
    if (scheduled_task_) {
      scheduled_task_->Cancel();
      scheduled_task_.reset();
    }
  }

  // QuartcTaskRunner::Task overrides.
  void Run() override {
    // The alarm may have been cancelled.
    if (!deadline().IsInitialized()) {
      return;
    }

    // The alarm may have been re-set to a later time.
    if (clock_->Now() < deadline()) {
      SetImpl();
      return;
    }

    Fire();
  }

 private:
  // Not owned by QuartcAlarm. Owned by the QuartcFactory.
  const net::QuicClock* clock_;
  // Not owned by QuartcAlarm. Owned by the QuartcFactory.
  net::QuartcTaskRunnerInterface* task_runner_;
  // Owned by QuartcAlarm.
  std::unique_ptr<net::QuartcTaskRunnerInterface::ScheduledTask> scheduled_task_;
};

FlyFactory::FlyFactory(const net::QuartcFactoryConfig& factory_config)
    : task_runner_(factory_config.task_runner),
      clock_(new FlyFactoryClock(factory_config.clock)) {}

FlyFactory::~FlyFactory() {}

std::unique_ptr<net::QuartcSession> FlyFactory::CreateFlyQuicSession(
    const FlySessionConf& session_config,
    const net::QuicConfig &quic_config,
    net::QuicSocketAddress &peer_addr) {
 // DCHECK(quartc_session_config.packet_transport);
  QuartcSessionConfig quartc_session_config;
  memcpy(&quartc_session_config,&session_config,sizeof(FlySessionConf));
  net::Perspective perspective = quartc_session_config.is_server
                                ? net::Perspective::IS_SERVER
                                : net::Perspective::IS_CLIENT;
  std::unique_ptr<net::QuicConnection> quic_connection =
      CreateFlyQuicConnection(quartc_session_config, perspective,peer_addr);
  net::QuicTagVector copt;
  if (quartc_session_config.congestion_control ==
      net::QuartcCongestionControl::kBBR) {
    copt.push_back(net::kTBBR);
  }
  quic_config.SetConnectionOptionsToSend(copt);
  quic_config.SetClientConnectionOptions(copt);
  return std::unique_ptr<net::QuartcSession>(new net::QuartcSession(
      std::move(quic_connection), quic_config,
      quartc_session_config.unique_remote_server_id, perspective,
      this /*QuicConnectionHelperInterface*/, clock_.get()));
}

std::unique_ptr<net::QuicConnection> FlyFactory::CreateFlyQuicConnection(
    const QuartcSessionConfig& quartc_session_config,
    net::Perspective perspective,
    net::QuicSocketAddress &peer_addr) {
  // The QuicConnection will take the ownership.
  std::unique_ptr<net::QuartcPacketWriter> writer(
      new net::QuartcPacketWriter(quartc_session_config.packet_transport,
                             quartc_session_config.max_packet_size));
  // dummy_id and dummy_address are used because Quartc network layer will not
  // use these two.
  //net::QuicConnectionId dummy_id = 0;
 // net::QuicSocketAddress dummy_address(net::QuicIpAddress::Any4(), 0 /*Port*/);
  return std::unique_ptr<net::QuicConnection>(new net::QuicConnection(
      connection_id++, peer_addr, this, /*QuicConnectionHelperInterface*/
      this /*QuicAlarmFactory*/, writer.release(), false /*own the writer*/,
      perspective, net::AllSupportedVersions()));
}

net::QuicAlarm* FlyFactory::CreateAlarm(net::QuicAlarm::Delegate* delegate) {
  return new FlyAlarm(GetClock(), task_runner_,
                         net::QuicArenaScopedPtr<net::QuicAlarm::Delegate>(delegate));
}

net::QuicArenaScopedPtr<net::QuicAlarm> FlyFactory::CreateAlarm(
    net::QuicArenaScopedPtr<net::QuicAlarm::Delegate> delegate,
    net::QuicConnectionArena* arena) {
  return net::QuicArenaScopedPtr<net::QuicAlarm>(
      new FlyAlarm(GetClock(), task_runner_, std::move(delegate)));
}

const net::QuicClock* FlyFactory::GetClock() const {
  return clock_.get();
}

net::QuicRandom* FlyFactory::GetRandomGenerator() {
  return net::QuicRandom::GetInstance();
}

net::QuicBufferAllocator* FlyFactory::GetBufferAllocator() {
  return &buffer_allocator_;
}

std::unique_ptr<FlyFactory> CreateFlyFactory(
    const net::QuartcFactoryConfig& factory_config) {
  return std::unique_ptr<FlyFactory>(
      new FlyFactory(factory_config));
}

}

}  // namespace net

