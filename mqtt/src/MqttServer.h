#ifndef MQTT_SERVER_H
#define MQTT_SERVER_H

#include "MqttApp.h"
#include "types/SessionInfo.h"
#include "types/Flag.h"

#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include <inet/common/socket/SocketMap.h>
#include <omnetpp.h>

namespace mqtt {
  using SubscriberInfo = std::pair<SessionInfo*, uint8_t>;

  class MqttServer : public MqttApp {
  protected:
    // parameters
    inet::L3Address localAddress;
    int localPort;

    int maxRetransmissions;
    double retransmissionInterval;

    std::map<int, std::string> socketToClient;
    std::map<std::string, SessionInfo> sessions;

    std::map<std::string, std::vector<SubscriberInfo>> wildcardSubscriptions;
    std::map<std::string, std::vector<SubscriberInfo>> subscriptions;    

    inet::SocketMap socketMap;
    
    omnetpp::cOutVector clientToServerDelay;

  protected:
    virtual void init(int stage) override;
    virtual void finish() override;

    virtual void handleStartOperation(inet::LifecycleOperation* operation) override;
    virtual void handleStopOperation(inet::LifecycleOperation* operation) override;
    virtual void handleCrashOperation(inet::LifecycleOperation* operation) override;

    virtual void socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent) override;
    virtual void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *info) override;
    
    virtual void handleMessageWhenUp(omnetpp::cMessage* msg) override;
    void handleRetransmission(inet::TcpSocket *socket, uint16_t packetId);

    virtual void processPacket(inet::Packet* pk, inet::TcpSocket *socket) override;
    virtual void processPacketByMessageType(inet::Packet* pk, inet::TcpSocket *socket, MsgType type);

    virtual bool processAckWithId(MsgType type, inet::TcpSocket *socket, uint16_t packetId, uint8_t reasonCode);
    virtual void processConnect(inet::Packet* pk, inet::TcpSocket *socket);
    virtual void processPingReq(inet::Packet* pk, inet::TcpSocket *socket);
    virtual void processDisconnect(inet::Packet* pk, inet::TcpSocket *socket);
    virtual void processPublish(inet::Packet* pk, inet::TcpSocket *socket);
    virtual void processPubRel(inet::Packet* pk, inet::TcpSocket *socket);
    virtual void processSubscribe(inet::Packet* pk, inet::TcpSocket *socket);
    virtual void processUnsubscribe(inet::Packet* pk, inet::TcpSocket *socket);
    virtual void processPubAck(inet::Packet* pk, inet::TcpSocket *socket);
    virtual void processPubRec(inet::Packet* pk, inet::TcpSocket *socket);
    virtual void processPubComp(inet::Packet* pk, inet::TcpSocket *socket);

    virtual void sendSubAck(inet::TcpSocket *socket, uint16_t packetId, uint8_t returnCode);
    virtual void sendUnsubAck(inet::TcpSocket *socket, uint16_t packetId, uint8_t reasonCode);

    virtual void sendPublish(inet::TcpSocket *socket, uint16_t packetId, bool dupFlag, uint8_t qos,
			     const std::string& topic, const std::string& data);

    void sendPacketIdWithReason(inet::TcpSocket *socket, MsgType type, uint16_t packetId, uint8_t reasonCode);
    void sendConnAck(inet::TcpSocket *socket, bool sessionExists, uint8_t reasonCode = (uint8_t)ConnectReasonCode::SUCCESS);
    void sendPubRec(inet::TcpSocket* socket, uint16_t packetId, uint8_t reasonCode);
    void sendPubAck(inet::TcpSocket* socket, uint16_t packetId, uint8_t reasonCode);
    void sendPubRel(inet::TcpSocket* socket, uint16_t packetId, uint8_t reasonCode);
    void sendPubComp(inet::TcpSocket* socket, uint16_t packetId, uint8_t reasonCode);
    void sendPingResp(inet::TcpSocket *socket);

    void scheduleRetransmission(SessionInfo *subscriber, uint16_t packetId, const MsgInfo &msg);
    
    virtual void cleanClientSession(inet::TcpSocket *socket);
    virtual SessionInfo* addNewClient(inet::TcpSocket *socket, std::string &clientId);
    virtual SessionInfo* getClientInfo(inet::TcpSocket *socket);


    virtual void dispatchPublishToSubscribers(MsgInfo& messageInfo);
    void sendAndSavePublish(SessionInfo *subscriber, MsgInfo& messageInfo, uint8_t publishQoS, std::string &publishedUnder);

    std::set<uint16_t> getUsedIds(SessionInfo *subscriber);
    bool hasWildcardCharacters(const std::string &topic);

    void insertSubscription(inet::TcpSocket *socket, const std::string &filter, uint8_t qos);
    void deleteSubscription(inet::TcpSocket *socket, const std::string &filter);
    
  public:
    MqttServer() {};
    ~MqttServer();
  };

} 

#endif // MQTT_SERVER_H
