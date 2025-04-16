#ifndef ZMQVEHICLEAPP_H_
#define ZMQVEHICLEAPP_H_

#include "inet/applications/base/ApplicationBase.h"
#include "inet/common/clock/ClockUserModuleMixin.h"
#include "./msg/ZmqMessage.h"

#include "ZmqSocket.h"

extern template class inet::ClockUserModuleMixin<inet::ApplicationBase>;

namespace zmq
{
  class ZmqVehicleApp : public inet::ClockUserModuleMixin<inet::ApplicationBase>, public inet::UdpSocket::ICallback {
  public:
      inet::UdpSocket publisherSocket;
      inet::UdpSocket subscriberSocket;

      std::vector<std::string> subscribedTopics;

      inet::L3Address destAddress;
      int destPort;
      int localPort;

      int publishDataSize;

      omnetpp::cOutVector endToEndDelay;

    ZmqVehicleApp();
    ~ZmqVehicleApp();

    long packetsReceived = 0;
    long packetsSent = 0;

    inet::ClockEvent *connectEvent = nullptr;
    inet::ClockEvent *publishEvent = nullptr;
    double publishInterval;

    // initialization
    virtual void initialize(int stage) override;

    // application base
    virtual void finish() override;
    virtual void refreshDisplay() const override;

    // socket handling
    virtual void socketDataArrived(inet::UdpSocket *socket, inet::Packet *packet) override;
    virtual void socketErrorArrived(inet::UdpSocket *socket, inet::Indication *indication) override;
    virtual void socketClosed(inet::UdpSocket *socket) override;
    virtual void configureSocket();

    // topics
    std::vector<std::string> getPredefinedTopics();

    // zmq
    void setupZmqConnections();
    void publishVehicleData();
    void handleZmqMessage(const ZmqMessage& msg);

    virtual void processZmqMessage(const ZmqMessage& msg);
    virtual inet::Packet* createZmqPacket(const ZmqMessage& msg);
    virtual ZmqMessage parseZmqPacket(inet::Packet* packet);
    bool shouldProcessTopic(std::string topic);
    std::string generatePublishData();
    std::string padData(const std::string& data, uint32_t requiredSize);

    virtual void handleMessageWhenUp(omnetpp::cMessage *msg) override;

    virtual void handleStartOperation(inet::LifecycleOperation* operation) override {}
    virtual void handleStopOperation(inet::LifecycleOperation* operation) override {}
    virtual void handleCrashOperation(inet::LifecycleOperation* operation) override {}
  };
}

#endif
