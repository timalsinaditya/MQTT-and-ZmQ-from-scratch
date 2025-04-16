#ifndef MQTTAPP_H_
#define MQTTAPP_H_

#include "inet/applications/base/ApplicationBase.h"
#include "inet/common/clock/ClockUserModuleMixin.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "types/MsgType.h"

extern template class inet::ClockUserModuleMixin<inet::ApplicationBase>;

namespace mqtt
{
  class MqttApp : public inet::ClockUserModuleMixin<inet::ApplicationBase>, public inet::TcpSocket::ICallback {
  protected:
    double retransmissionInterval;
    int maxRetransmissions;
    double packetBER;

    inet::TcpSocket localSocket;

    long packetsReceived = 0;
    long packetsSent = 0;
    long erroneousPackets = 0;
    long numRetransmission = 0;

    // initialization
    virtual void initialize(int stage) override;

    // application base
    virtual void finish() override;
    virtual void refreshDisplay() const override;

    // socket handling
    virtual void socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *info) override {}
    virtual void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *info) override {}
    virtual void socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent) override {}
    virtual void socketEstablished(inet::TcpSocket *socket) override {}
    virtual void socketPeerClosed(inet::TcpSocket *socket) override {}
    virtual void socketClosed(inet::TcpSocket *socket) override;
    virtual void socketFailure(inet::TcpSocket *socket, int code) override {}
    virtual void socketDeleted(inet::TcpSocket *socket) override {}
    virtual void configureSocket();

    // packet handling
    virtual void checkPacketIntegrity(const inet::B& receivedLen, const inet::B& fieldLen);
    virtual void corruptPacket(inet::Packet *packet, double ber);
    virtual bool hasProbabilisticError(inet::b length, double ber);
    virtual bool isSelfBroadcastAddress(const inet::L3Address& address);

    virtual uint16_t getNextPacketId(const std::set<uint16_t>& used, uint16_t &current, const std::string& error = "Cannot find unused id");

    // topics
    std::vector<std::string> getPredefinedTopics();
    
    // child class should implement
    virtual void init(int stage) = 0;
    virtual void processPacket(inet::Packet *packet, inet::TcpSocket *socket) = 0;
  };
}

#endif // MQTTAPP_H_
