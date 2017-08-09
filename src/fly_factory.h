#ifndef _FLY_FACTORY_H_
#define _FLY_FACTORY_H_

#include "../libquic/include/net/quic/core/quic_alarm_factory.h"
#include "../libquic/include/net/quic/core/quic_connection.h"
#include "../libquic/include/net/quic/core/quic_simple_buffer_allocator.h"
#include "../libquic/include/net/quic/platform/api/quic_export.h"
#include "../libquic/include/net/quic/quartc/quartc_factory_interface.h"
#include "../libquic/include/net/quic/quartc/quartc_packet_writer.h"
#include "../libquic/include/net/quic/quartc/quartc_task_runner_interface.h"
//#include "fly_quic_session.h"

namespace fly {

struct FlySessionConf{
//	public:
    // When using Quartc, there are two endpoints. The QuartcSession on one
    // endpoint must act as a server and the one on the other side must act as a
    // client.
    bool is_server = false;
    // This is only needed when is_server = false.  It must be unique
    // for each endpoint the local endpoint may communicate with. For example,
    // a WebRTC client could use the remote endpoint's crypto fingerprint
    std::string unique_remote_server_id;
    // The way the QuicConnection will send and receive packets, like a virtual
    // UDP socket. For WebRTC, this will typically be an IceTransport.
    net::QuartcSessionInterface::PacketTransport* packet_transport = nullptr;
    // The maximum size of the packet can be written with the packet writer.
    // 1200 bytes by default.
    uint64_t max_packet_size = 1200;
    // Algorithm to use for congestion control.  By default, uses an arbitrary
    // congestion control algorithm chosen by QUIC.
     net::QuartcCongestionControl congestion_control =
         net::QuartcCongestionControl::kDefault;
};


#if 0
class FlyClock : public net::QuicClock {
 public:
  FlyClock();
  ~FlyClock() override;

  // QuicClock implementation:
  net::QuicTime Now() const override;
  net::QuicTime ApproximateNow() const override;
  net::QuicWallTime WallNow() const override;

  // Advances the current time by |delta|, which may be negative.
  void AdvanceTime(net::QuicTime::Delta delta);

  // Returns the current time in ticks.
  base::TimeTicks NowInTicks() const;

 private:
  net::QuicTime now_;

};
#endif
// Adapts QuartcClockInterface (provided by the user) to QuicClock
// (expected by QUIC).
class FlyFactoryClock : public net::QuicClock {
 public:
  explicit FlyFactoryClock(net::QuartcClockInterface* clock) : clock_(clock) {}
  net::QuicTime ApproximateNow() const override { return Now(); }
   net::QuicTime Now() const override {
    return  net::QuicTime::Zero() +
            net::QuicTime::Delta::FromMicroseconds(clock_->NowMicroseconds());
  }
   net::QuicWallTime WallNow() const override {
    return  net::QuicWallTime::FromUNIXMicroseconds(clock_->NowMicroseconds());
  }

 private:
   net::QuartcClockInterface* clock_;
};



// Implements the QuartcFactoryInterface to create the instances of
// QuartcSessionInterface. Implements the QuicAlarmFactory to create alarms
// using the QuartcTaskRunner. Implements the QuicConnectionHelperInterface used
// by the QuicConnections. Only one QuartcFactory is expected to be created.
class  FlyFactory : public net::QuartcFactoryInterface,
                                          public net::QuicAlarmFactory,
                                          public net::QuicConnectionHelperInterface {
 public:
  explicit FlyFactory(const net::QuartcFactoryConfig& factory_config);
  ~FlyFactory() override;

  std::unique_ptr<net::QuartcSession> CreateFlyQuicSession(
      const FlySessionConf &quartc_session_config,
      const net::QuicConfig &quic_config,
      net::QuicSocketAddress &peer_addr);
      
  // QuicAlarmFactory overrides.
  net::QuicAlarm* CreateAlarm(net::QuicAlarm::Delegate* delegate) override;

  net::QuicArenaScopedPtr<net::QuicAlarm> CreateAlarm(
      net::QuicArenaScopedPtr<net::QuicAlarm::Delegate> delegate,
      net::QuicConnectionArena* arena) override;

  // QuicConnectionHelperInterface overrides.
  const net::QuicClock* GetClock() const override;

  net::QuicRandom* GetRandomGenerator() override;

  net::QuicBufferAllocator* GetBufferAllocator() override;

 private:
  std::unique_ptr<net::QuicConnection> CreateFlyQuicConnection(
      const QuartcSessionConfig& quartc_session_config,
      net::Perspective perspective,
      net::QuicSocketAddress &peer_addr);

	
	// QuartcFactoryInterface overrides.
	 std::unique_ptr<net::QuartcSessionInterface> CreateQuartcSession(
		 const QuartcSessionConfig& quartc_session_config) override;
		 
    std::unique_ptr<net::QuicConnection> CreateQuicConnection(
      const QuartcSessionConfig& quartc_session_config,
      net::Perspective perspective);

  // Used to implement QuicAlarmFactory..
  net::QuartcTaskRunnerInterface* task_runner_;
  // Used to implement the QuicConnectionHelperInterface.
  // The QuicClock wrapper held in this variable is owned by QuartcFactory,
  // but the QuartcClockInterface inside of it belongs to the user!
  std::unique_ptr<net::QuicClock> clock_;
  net::SimpleBufferAllocator buffer_allocator_;
};

std::unique_ptr<FlyFactory> CreateFlyFactory(
    const net::QuartcFactoryConfig& factory_config);



}  // namespace net


#endif
