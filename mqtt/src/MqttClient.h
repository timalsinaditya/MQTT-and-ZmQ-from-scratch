#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "MqttApp.h"
#include "types/MsgInfo.h"
#include "types/Flag.h"
#include <omnetpp.h>

namespace mqtt
{
  class MqttClient : public MqttApp {
  protected:
    inet::L3Address serverAddress;
    int serverPort;

    uint16_t keepAlive;
    bool cleanSession;
    bool isConnected = false;

    inet::ClockEvent *connectEvent = nullptr;
    inet::ClockEvent *publishEvent = nullptr;
    inet::ClockEvent *subscriptionEvent = nullptr;
    inet::ClockEvent *unsubscriptionEvent = nullptr;
    inet::ClockEvent *pingEvent = nullptr;
    double pingEventInterval;
    double reconnectInterval;
    double publishInterval;
    double subscriptionInterval;
    double unsubscriptionInterval;
    int publishDataSize;

    std::vector<std::string> toSubscribe;
    std::vector<std::string> subscribedTo;
    std::vector<std::string> publishTopics;

    int maxRetransmissions;

    uint16_t lastPacketId;
    std::string clientId;
    std::map<uint16_t, RetransmissionInfo> retransmissions;
    std::map<uint16_t, MsgInfo> receivedQos2Messages;

    omnetpp::cOutVector serverToClientDelay;

    omnetpp::cOutVector publishPacketSize;
    omnetpp::cOutVector subscribePacketSize;
    omnetpp::cOutVector connectPacketSize;
    omnetpp::cOutVector pingPacketSize;

    long publishPacketSent = 0;
    long subscribePacketSent = 0;
    long connectPacketSent = 0;
    long pingPacketSent = 0;

    virtual void init(int stage) override;
    virtual void finish() override;

    virtual void handleStartOperation(inet::LifecycleOperation* operation) override;
    virtual void handleStopOperation(inet::LifecycleOperation* operation) override;
    virtual void handleCrashOperation(inet::LifecycleOperation* operation) override;
    
    virtual void socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent) override;
    virtual void socketEstablished(inet::TcpSocket *socket) override;
    virtual void socketPeerClosed(inet::TcpSocket *socket) override;
    virtual void socketClosed(inet::TcpSocket *socket) override;
    virtual void socketFailure(inet::TcpSocket *socket, int code) override;
    virtual void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *info) override;

    virtual std::string generatePublishData();
    virtual std::string padData(const std::string& data, uint32_t requiredSize);
    virtual void handleMessageWhenUp(omnetpp::cMessage *msg) override;
    void handleConnectEvent();
    void handlePingEvent();
    void handleSubscriptionEvent();
    void handleUnsubscriptionEvent();
    void handlePublishEvent();
    void handleRetransmission(uint16_t packetId);
    void scheduleRetransmission(uint16_t packetId, const MsgInfo &msg);

    virtual void processPacket(inet::Packet *pk, inet::TcpSocket *socket) override;
    virtual void processPacketByMsgType(inet::Packet *pk, MsgType type);
    void processAckWithId(MsgType type, uint16_t packetId, uint8_t reasonCode);

    virtual void processConnAck(inet::Packet *pk);
    virtual void processPingResp();
    virtual void processDisconnect(inet::Packet *pk);
    virtual void processSubAck(inet::Packet *pk);
    virtual void processUnsubAck(inet::Packet *pk);
    virtual void processPublish(inet::Packet *pk);
    virtual void processPubAck(inet::Packet *pk);
    virtual void processPubRec(inet::Packet *pk);
    virtual void processPubRel(inet::Packet *pk);
    virtual void processPubComp(inet::Packet *pk);

    virtual void sendPacketIdWithReason(MsgType type, uint16_t packetId, uint8_t reasonCode);
    virtual void sendConnect(uint16_t keepAlive, uint8_t willQos = 0,
			     const std::string &willTopic = "", const std::string &willMsg = "",
			     const std::string &username = "", const std::string &password = "");
    virtual void sendPublish(uint16_t packetId, uint8_t qos, bool dupFlag,
			     const std::string &topic, const std::string &data);
    virtual void sendPingReq();
    virtual void sendSubscribe(uint16_t packetId, std::string &topicFilter,
			       uint8_t options = (uint8_t)SubOpts::QOS_TWO);
    virtual void sendUnsubscribe(uint16_t packetId, std::string &topicFilter);
    void sendPubRec(uint16_t packetId, uint8_t reasonCode);
    void sendPubAck(uint16_t packetId, uint8_t reasonCode);
    void sendPubRel(uint16_t packetId, uint8_t reasonCode);
    void sendPubComp(uint16_t packetId, uint8_t reasonCode);

    std::set<uint16_t> getUsedPacketIds();

    void clearRetransmissions();
    void clearReceivedQos2Messages();

    std::string getClientId();

  public:
    MqttClient();
    ~MqttClient();    
  };
}

#endif // MQTT_CLIENT_H
