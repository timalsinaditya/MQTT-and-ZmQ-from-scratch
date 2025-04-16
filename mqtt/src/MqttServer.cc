#include "MqttServer.h"
#include "msg/MqttConnect.h"
#include "msg/MqttFixed.h"
#include "msg/MqttDisconnect.h"
#include "msg/MqttPingReq.h"
#include "msg/MqttPublish.h"
#include "msg/MqttSubscribe.h"
#include "msg/MqttSubAck.h"
#include "msg/MqttUnsubAck.h"
#include "msg/MqttUnsubscribe.h"
#include "msg/MqttPubAck.h"
#include "msg/MqttPubRec.h"
#include "msg/MqttPubRel.h"
#include "msg/MqttPubComp.h"
#include "msg/MqttPingResp.h"
#include "msg/MqttConnAck.h"

#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"

using std::endl;

namespace mqtt {

  Define_Module(MqttServer);
  Register_Class(MqttServer);

  void MqttServer::init(int stage)
  {
    if(stage == inet::INITSTAGE_APPLICATION_LAYER) {
        clientToServerDelay.setName("Client to Server Latency");
        localAddress = inet::L3AddressResolver().resolve(par("localAddress"));
        MqttApp::configureSocket();
        localSocket.listen();
    }
    localPort = par("localPort");
  }

  void MqttServer::finish()
  {
    sessions.clear();
    wildcardSubscriptions.clear();
    subscriptions.clear();
    socketToClient.clear();
    // TODO : record stats
    
    recordScalar("Packets Sent", packetsSent);
    recordScalar("Packets Received", packetsReceived);
    recordScalar("Retransmissions", numRetransmission);
    recordScalar("Erroneous Packets", erroneousPackets);

    MqttApp::finish();
  }

  void MqttServer::socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent) {
    processPacket(packet, socket);
  }

  void MqttServer::socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *info) {
    if(socket != &localSocket) {
      return;
    }
    
    inet::TcpSocket *newSocket = new inet::TcpSocket(info);
    newSocket->setOutputGate(gate("socketOut"));
    newSocket->setCallback(this);

    // TODO : add to a list of sockets that are waiting for first connect packets

    socketMap.addSocket(newSocket);
    socket->accept(info->getNewSocketId());
  }

  void MqttServer::handleStartOperation(inet::LifecycleOperation* operation)
  {
  }

  void MqttServer::handleStopOperation(inet::LifecycleOperation* operation)
  {
    sessions.clear();
    wildcardSubscriptions.clear();
    subscriptions.clear();
    socketToClient.clear();
    
    MqttApp::localSocket.close();
  }

  void MqttServer::handleCrashOperation(inet::LifecycleOperation* operation)
  {
    sessions.clear();
    wildcardSubscriptions.clear();
    subscriptions.clear();
    socketToClient.clear();
    
    MqttApp::localSocket.destroy();
  }

  void MqttServer::handleMessageWhenUp(omnetpp::cMessage* msg)
  {
    if (msg->hasPar("isRetransmission")) {
      inet::TcpSocket *socket = (inet::TcpSocket*)msg->par("socket").pointerValue();
      SessionInfo *info = getClientInfo(socket);
      if(info == nullptr) {
	EV_ERROR << "Retransmission Event for socket without a session" << endl;
	delete msg;
	return;
      }

      uint16_t packetId = msg->par("packetId");
      auto &retransmissions = info->retransmissions;
      if(retransmissions.find(packetId) != retransmissions.end()) {
	handleRetransmission(socket, packetId);
      }
    }
    else {
      if (localSocket.belongsToSocket(msg))
	localSocket.processMessage(msg);
      else {
	inet::TcpSocket *socket = inet::check_and_cast_nullable<inet::TcpSocket*>(socketMap.findSocketFor(msg));
        if (socket)
	  socket->processMessage(msg);
        else {
	  EV_ERROR << "message " << msg->getFullName() << "(" <<
	    msg->getClassName() << ") arrived for unknown socket \n";
	  delete msg;
        }
      }
    }
  }

  void MqttServer::handleRetransmission(inet::TcpSocket *socket, uint16_t packetId) {
    // TODO : maybe assert it exists ?

    numRetransmission++;
    SessionInfo &session = sessions[socketToClient[socket->getSocketId()]];
    RetransmissionInfo &info = session.retransmissions[packetId];
    if (info.retransmissionCounter >= maxRetransmissions) {
      EV << "Max retransmissions reached for packet " << packetId << ". Giving up." << endl;
      session.retransmissions.erase(packetId);
      return;
    }
    info.retransmissionCounter++;

    switch (info.msg.type) {
    case MsgType::PUBLISH:
      sendPublish(socket, packetId, info.msg.qos, true, info.msg.topic, info.msg.data);
      break;
    case MsgType::PUBREC:
      sendPubRec(socket, packetId, info.msg.reasonCode);
      break;
    case MsgType::PUBREL:
      sendPubRel(socket, packetId, info.msg.reasonCode);
      break;
    default:
      EV_ERROR << "Unexpected message type for retransmission: " << (int)info.msg.type << endl;
      return;
    }

    scheduleClockEventAfter(retransmissionInterval, info.retransmissionEvent);
  }

  void MqttServer::processPacket(inet::Packet* pk, inet::TcpSocket *socket) {
      packetsReceived++;

      if (pk->hasBitError()) {
          erroneousPackets++;
          delete pk;
          return;
      }

      // Try to get the packet data as a SequenceChunk
      const auto& chunk = pk->peekDataAt<inet::Chunk>(inet::B(0), pk->getTotalLength());
      const inet::SequenceChunk* sequenceChunk = dynamic_cast<const inet::SequenceChunk*>(chunk.get());

      if (sequenceChunk) {
          EV << "Processing SequenceChunk at Server" << endl;

          const std::deque<inet::Ptr<const inet::Chunk>>& chunks = sequenceChunk->getChunks();

          for (const auto& subchunk : chunks) {
              const auto* header = dynamic_cast<const MqttFixed*>(subchunk.get());

              if (header) {
                  // Validate packet integrity
                  MqttApp::checkPacketIntegrity((inet::B) subchunk->getChunkLength(), (inet::B) header->getRemLength());

                  MsgType type = header->getMsgType();
                  inet::Packet* newPacket = new inet::Packet();
                  newPacket->insertAtBack(subchunk);

                  if (newPacket->hasBitError()) {
                      erroneousPackets++;
                      delete newPacket;
                      continue;
                  }
                  processPacketByMessageType(newPacket, socket, type);
                  packetsReceived++;
              } else {
                  EV_WARN << "Subchunk is not a valid MqttFixed header!" << endl;
              }
          }
      }
      else {
          // Fallback for non-SequenceChunk packets
          const auto* header = dynamic_cast<const MqttFixed*>(pk->peekData<inet::Chunk>().get());

          if (!header) {
              EV_ERROR << "Packet does not contain a valid MqttFixed header!" << endl;
              delete pk;
              return;
          }

          MqttApp::checkPacketIntegrity((inet::B) pk->getByteLength(), (inet::B) header->getRemLength());

          EV << "Packet Received by Server" << endl;

          MsgType msgType = header->getMsgType();
          processPacketByMessageType(pk, socket, msgType);
          packetsReceived++;
      }

      delete pk;
  }


  void MqttServer::processPacketByMessageType(inet::Packet* pk, inet::TcpSocket *socket, MsgType type) {
    switch(type) {
    case MsgType::CONNECT:
      processConnect(pk, socket);
      break;

    case MsgType::PINGREQ:
      processPingReq(pk, socket);
      break;

    case MsgType::DISCONNECT:
      processDisconnect(pk, socket);
      break;

    case MsgType::PUBLISH:
      processPublish(pk, socket);
      break;

    case MsgType::PUBACK:
      processPubAck(pk, socket);
      break;

    case MsgType::PUBREL:
      processPubRel(pk, socket);
      break;

    case MsgType::PUBREC:
      processPubRec(pk, socket);
      break;

    case MsgType::PUBCOMP:
      processPubComp(pk, socket);
      break;

    case MsgType::SUBSCRIBE:
      processSubscribe(pk, socket);
      break;

    case MsgType::UNSUBSCRIBE:
      processUnsubscribe(pk, socket);
      break;

    default:
      break;
    }
  }

  void MqttServer::processConnect(inet::Packet* pk, inet::TcpSocket *socket)
  {
    const auto& payload = pk->peekData<MqttConnect>();

    if (payload->getProtoVersion() != 0x05) {
      sendConnAck(socket, false, (uint8_t)ConnectReasonCode::UNSUPPORTED_PROTOCOL_VERSION);
      return;
    }

    std::string clientId = payload->getClientId();

    // TODO : handle what happens when a session exists
    const auto &client = sessions.find(clientId);
    bool sessionExists = client != sessions.end();

    SessionInfo *clientInfo;
    
    if (sessionExists) {
      if (payload->getFlag(ConnectFlag::CLEAN_START, payload->connectFlags)) {
	cleanClientSession(socket);
      }
      clientInfo = &client->second;
    }
    else {
      clientInfo = addNewClient(socket, clientId);
    }

    clientInfo->keepAlive = payload->getKeepAlive();

    uint8_t will = payload->getFlag(ConnectFlag::WILL_FLAG, payload->connectFlags);
    uint8_t willQos = payload->getFlag(ConnectFlag::WILL_QOS, payload->connectFlags);

    // TODO : handle will messages

    sendConnAck(socket, sessionExists);
  }

  void MqttServer::processPingReq(inet::Packet* pk, inet::TcpSocket *socket)
  {
    const auto& payload = pk->peekData<MqttPingReq>();
    sendPingResp(socket);
  }

  void MqttServer::sendConnAck(inet::TcpSocket *socket, bool sessionExists, uint8_t reasonCode) {
      const auto &payload = inet::makeShared<MqttConnAck>();
      payload->setMsgType(CONNACK);
      payload->setFlag(ConnAckFlag::SESSION_PRESENT, (uint8_t)sessionExists, payload->connAckFlags);
      payload->setReasonCode((ReasonCode)reasonCode);
      payload->setChunkLength(inet::B(payload->getRemLength() + 2));

      inet::Packet* packet = new inet::Packet("ConnAckPacket");
      packet->insertAtBack(payload);
      MqttApp::corruptPacket(packet, packetBER);
      packetsSent++;

      socket->send(packet);
  }

  void MqttServer::processDisconnect(inet::Packet* pk, inet::TcpSocket *socket)
  {
    const auto& payload = pk->peekData<MqttDisconnect>();

    uint8_t reasonCode = (uint8_t)payload->getReasonCode();
    if(reasonCode == (uint8_t)DisconnectReasonCode::NORMAL_DISCONNECTION ||
       reasonCode == (uint8_t)DisconnectReasonCode::DISCONNECT_WITH_WILL) {
      EV << "Client " << socketToClient[socket->getSocketId()] << " disconnected normally." << endl;
      if(reasonCode == (uint8_t)DisconnectReasonCode::DISCONNECT_WITH_WILL) {
	// TODO : handle will message
      }
    } else {
      EV_ERROR << "Client " << socketToClient[socket->getSocketId()] << " disconnected abnormally." << reasonCode << endl;
    }
  }

  void MqttServer::processPublish(inet::Packet* pk, inet::TcpSocket *socket) 
  {
      auto chunk = pk->peekData<inet::Chunk>();

      // Check if the chunk is a SliceChunk
      auto sliceChunk = inet::dynamicPtrCast<const inet::SliceChunk>(chunk);
      inet::Ptr<const MqttPublish> payload;

      if (sliceChunk) {
          // Extract the original chunk from the SliceChunk
          auto originalChunk = sliceChunk->getChunk();
          payload = inet::dynamicPtrCast<const MqttPublish>(originalChunk);
      } else {
          // Directly try to cast the chunk if it's not a SliceChunk
          payload = inet::dynamicPtrCast<const MqttPublish>(chunk);
      }

      if(!payload) { return; }
    
    // TODO : handle timing stats

    double creationTime = payload->getCreationTime();
    double delay = omnetpp::simTime().dbl() - creationTime;
    clientToServerDelay.record(delay);

    MsgInfo messageInfo;
    messageInfo.topic = payload->getTopicName();
    messageInfo.dup = payload->getFlag(Flag::DUP, payload->flags);
    messageInfo.qos = payload->getFlag(Flag::QOS, payload->flags);
    messageInfo.retain = payload->getFlag(Flag::RETAIN, payload->flags);
    messageInfo.data = payload->getPayload();
    messageInfo.packetId = payload->getPacketIdentifier();

    if (messageInfo.qos == 0) {
      dispatchPublishToSubscribers(messageInfo);
      return;
    }

    if (messageInfo.qos == 1) {
      dispatchPublishToSubscribers(messageInfo);
      sendPubAck(socket, messageInfo.packetId, (uint8_t)PubAckReasonCode::SUCCESS);
      return;
    }

    sessions[socketToClient[socket->getSocketId()]].receivedQos2Messages[messageInfo.packetId] = messageInfo;
    sendPubRec(socket, messageInfo.packetId, (uint8_t)PubAckReasonCode::SUCCESS);
  }

  void MqttServer::processSubscribe(inet::Packet* pk, inet::TcpSocket *socket)
  {
    if (socketToClient.find(socket->getSocketId()) == socketToClient.end()) {
      return;
    }
    
    const auto& payload = pk->peekData<MqttSubscribe>();
    uint16_t packetId = payload->getPacketIdentifier();
    std::pair<MqttString, uint8_t> topicFilter = payload->getTopicFilters()[0]; // only one per message
    
    uint8_t subOpts = topicFilter.second;
    const std::string &filter = topicFilter.first.get();
    
    uint8_t qos = subOpts & 0x03;
    if(qos == 3) {
      sendSubAck(socket, packetId, (uint8_t)SubscribeReasonCode::UNSPECIFIED_ERROR);
      return;
    }
    
    insertSubscription(socket, filter, qos);
    sendSubAck(socket, packetId, qos);
  }

  void MqttServer::processUnsubscribe(inet::Packet* pk, inet::TcpSocket *socket)
  {
    const auto& payload = pk->peekData<MqttUnsubscribe>();
    uint16_t packetId = payload->getPacketIdentifier();
    const std::string topicFilter  = payload->getTopicFilters()[0].get();

    // TODO : send no_sub_existed 
    deleteSubscription(socket, topicFilter);

    sendUnsubAck(socket, packetId, (uint8_t)UnsubscribeReasonCode::SUCCESS);
  }

  bool MqttServer::processAckWithId(MsgType type, inet::TcpSocket *socket, uint16_t packetId, uint8_t reasonCode) {
    SessionInfo *session = getClientInfo(socket);
    if(session == nullptr) return false;
    
    uint8_t successCode = 0;
    switch(type) {
    case PUBACK: successCode = (uint8_t) PubAckReasonCode::SUCCESS; break;
    case PUBREC: successCode = (uint8_t) PubAckReasonCode::SUCCESS; break;
    case PUBREL: successCode = (uint8_t) PubRelReasonCode::SUCCESS; break;
    case PUBCOMP: successCode = (uint8_t) PubRelReasonCode::SUCCESS; break;      
    default: break;
    }

    auto &retransmissions = session->retransmissions;
    if(retransmissions.find(packetId) == retransmissions.end()) {
      EV_WARN << "Ignoring Unexpected Pulish Acknowledgement: ID = " << packetId << endl;
      return false;
    }

    cancelAndDelete(retransmissions[packetId].retransmissionEvent);
    retransmissions.erase(packetId);
    
    if(reasonCode != successCode) {
      throw omnetpp::cRuntimeError("Server Returned Error: %d", reasonCode);
    }

    return true;
  }

  void MqttServer::processPubAck(inet::Packet* pk, inet::TcpSocket *socket)
  {
    const auto& payload = pk->peekData<MqttPubAck>();
    uint16_t packetId = payload->getPacketIdentifier();
    uint8_t reasonCode = (uint8_t)payload->getReasonCode();
    processAckWithId(MsgType::PUBACK, socket, packetId, reasonCode);
  }

  void MqttServer::processPubRec(inet::Packet* pk, inet::TcpSocket *socket)
  {
    const auto& payload = pk->peekData<MqttPubRec>();
    uint16_t packetId = payload->getPacketIdentifier();
    uint8_t reasonCode = (uint8_t)payload->getReasonCode();
    if(!processAckWithId(MsgType::PUBREC, socket, packetId, reasonCode)) {
      return;
    }

    sendPubRel(socket, packetId, reasonCode);
    // TODO : reschedule pub rel
  }

  void MqttServer::processPubRel(inet::Packet* pk, inet::TcpSocket *socket)
  {
    const auto& payload = pk->peekData<MqttPubRel>();
    uint16_t packetId = payload->getPacketIdentifier();
    uint8_t reasonCode = (uint8_t)payload->getReasonCode();
    if(!processAckWithId(MsgType::PUBREL, socket, packetId, reasonCode)) {
      return;
    } 
    
    std::map<uint16_t, MsgInfo> messages = sessions[socketToClient[socket->getSocketId()]].receivedQos2Messages;

    const auto &messageIt = messages.find(packetId);
    if (messageIt == messages.end()) {
      return;
    }
     
    MsgInfo& messageInfo = messageIt->second;
    dispatchPublishToSubscribers(messageInfo);
    
    messages.erase(messageIt);

    sendPubComp(socket, packetId, (uint8_t)PubRelReasonCode::SUCCESS);
  }

  void MqttServer::processPubComp(inet::Packet* pk, inet::TcpSocket *socket)
  {
    const auto& payload = pk->peekData<MqttPubComp>();
    uint16_t packetId = payload->getPacketIdentifier();
    uint8_t reasonCode = (uint8_t)payload->getReasonCode();
    processAckWithId(MsgType::PUBCOMP, socket, packetId, reasonCode);
  }

  
  void MqttServer::sendPacketIdWithReason(inet::TcpSocket *socket, MsgType type, uint16_t packetId, uint8_t reasonCode) {
    const auto &payload = inet::makeShared<MqttPubAck>();
    payload->setMsgType(type);
    payload->setPacketIdentifier(packetId);
    payload->setReasonCode((ReasonCode)reasonCode);
    payload->setChunkLength(inet::B(payload->getRemLength() + 2));

    const char *packetName;
    switch(type) {
    case MsgType::PUBACK : packetName = "PubAckPacket"; break;
    case MsgType::PUBREC : packetName = "PubRecPacket"; break;
    case MsgType::PUBREL : packetName = "PubRelPacket"; break;
    case MsgType::PUBCOMP : packetName = "PubCompPacket"; break;      
    default: throw omnetpp::cRuntimeError("Unexpected packet type : %u", type);
    }
    inet::Packet *packet = new inet::Packet(packetName);
    packet->insertAtBack(payload);
    MqttApp::corruptPacket(packet, packetBER);
    packetsSent++;
    
    socket->send(packet);
  }

  void MqttServer::sendPubAck(inet::TcpSocket *socket, uint16_t packetId, uint8_t reasonCode) {
    sendPacketIdWithReason(socket, MsgType::PUBACK, packetId, reasonCode);
  }

  void MqttServer::sendPubRec(inet::TcpSocket *socket, uint16_t packetId, uint8_t reasonCode) {
    sendPacketIdWithReason(socket, MsgType::PUBREC, packetId, reasonCode);
  }

  void MqttServer::sendPubRel(inet::TcpSocket *socket, uint16_t packetId, uint8_t reasonCode) {
    sendPacketIdWithReason(socket, MsgType::PUBREL, packetId, reasonCode);
  }

  void MqttServer::sendPubComp(inet::TcpSocket *socket, uint16_t packetId, uint8_t reasonCode) {
    sendPacketIdWithReason(socket, MsgType::PUBCOMP, packetId, reasonCode);
  }

  void MqttServer::sendSubAck(inet::TcpSocket *socket, uint16_t packetId, uint8_t reasonCode)
  {
    const auto& payload = inet::makeShared<MqttSubAck>();
    payload->setMsgType(MsgType::SUBACK);
    payload->setPacketIdentifier(packetId);
    payload->addReasonCode((SubscribeReasonCode)reasonCode);
    payload->setChunkLength(inet::B(payload->getRemLength() + 2));

    inet::Packet* packet = new inet::Packet("SubAckPacket");
    packet->insertAtBack(payload);
    MqttApp::corruptPacket(packet, packetBER);
    packetsSent++;

    socket->send(packet);
  }

  void MqttServer::sendUnsubAck(inet::TcpSocket *socket, uint16_t packetId, uint8_t reasonCode)
  {
    const auto& payload = inet::makeShared<MqttUnsubAck>();
    payload->setMsgType(MsgType::UNSUBACK);
    payload->setPacketIdentifier(packetId);
    payload->addReasonCode((UnsubscribeReasonCode)reasonCode);
    payload->setChunkLength(inet::B(payload->getRemLength() + 2));

    inet::Packet* packet = new inet::Packet("UnsubAckPacket");
    packet->insertAtBack(payload);
    MqttApp::corruptPacket(packet, packetBER);
    packetsSent++;

    socket->send(packet);
  }

  
  void MqttServer::sendPingResp(inet::TcpSocket *socket)
  {
    const auto& payload = inet::makeShared<MqttPingResp>();
    payload->setMsgType(MsgType::PINGRESP);
    payload->setChunkLength(inet::B(payload->getRemLength() + 2));

    inet::Packet* packet = new inet::Packet("PingRespPacket");
    packet->insertAtBack(payload);
    MqttApp::corruptPacket(packet, packetBER);
    packetsSent++;

    socket->send(packet);
  }

  void MqttServer::sendPublish(inet::TcpSocket *socket, uint16_t packetId, bool dupFlag, uint8_t qos,
                               const std::string &topic, const std::string &data)
  {
    const auto& payload = inet::makeShared<MqttPublish>();
    payload->setMsgType(MsgType::PUBLISH);
    payload->setFlag(Flag::DUP, (uint8_t)dupFlag, payload->flags);
    payload->setFlag(Flag::QOS, qos, payload->flags);
    payload->setFlag(Flag::RETAIN, 0, payload->flags); // Retained messages not supported
    payload->setPacketIdentifier(packetId);
    payload->setTopicName(topic);
    payload->addPayload(data);
    payload->setChunkLength(inet::B(payload->getRemLength() + 2));
    payload->setCreationTime(omnetpp::simTime().dbl());

    // TODO : timestamp the packet ?

    inet::Packet *packet = new inet::Packet("PublishPacket");
    packet->insertAtBack(payload);
    corruptPacket(packet, packetBER);
    packetsSent++;

    socket->send(packet);
  }

  void MqttServer::cleanClientSession(inet::TcpSocket *socket)
  {
    if (socketToClient.find(socket->getSocketId()) == socketToClient.end()) {
      throw omnetpp::cRuntimeError("Client not found while cleaning session");
    }
    // TODO : handle Will messages
    // TODO : delete subscriptions
  }

  SessionInfo* MqttServer::addNewClient(inet::TcpSocket *socket, std::string &clientId)
  {
    socketToClient[socket->getSocketId()] = clientId;    

    SessionInfo session;
    session.socket = socket;
    session.clientId = clientId;
    sessions[clientId] = session;

    return &sessions[clientId];
  }

  SessionInfo* MqttServer::getClientInfo(inet::TcpSocket *socket)
  {
    auto clientId = socketToClient.find(socket->getSocketId());
    if(clientId == socketToClient.end()) return nullptr;
    
    const auto &clientInfo = sessions.find(clientId->second);
    if (clientInfo == sessions.end()) return nullptr;

    return &clientInfo->second;
  }

  uint8_t min(uint8_t a, uint8_t b) {
    return a < b ? a : b; 
  }
  
  void MqttServer::dispatchPublishToSubscribers(MsgInfo& messageInfo)
  {
    std::string &publishTopic = messageInfo.topic;

    for (auto &subscription : wildcardSubscriptions) {
      // TODO : handle wildcard subscription matching
    }

    const auto &subscription = subscriptions.find(publishTopic);
    if(subscription == subscriptions.end()) {
      return;
    }

    std::vector<SubscriberInfo> &subscribers = subscription->second;
    for (auto &subscriberInfo : subscribers) {
      SessionInfo *subscriber = subscriberInfo.first;
      uint8_t qos = subscriberInfo.second;

      uint8_t publishQos = min(qos, messageInfo.qos);
      sendAndSavePublish(subscriber, messageInfo, publishQos, publishTopic);
    }
  }

  void MqttServer::sendAndSavePublish(SessionInfo *subscriber, MsgInfo& messageInfo, uint8_t publishQos, std::string &publishedUnder)
  {
    uint16_t packetId = MqttApp::getNextPacketId(getUsedIds(subscriber), subscriber->lastPacketId);
    if (publishQos == 1 || publishQos == 2) {
      messageInfo.type = MsgType::PUBLISH;
    }

    sendPublish(subscriber->socket, packetId, messageInfo.dup, messageInfo.qos, publishedUnder, messageInfo.data);
  }

  void MqttServer::scheduleRetransmission(SessionInfo *subscriber, uint16_t packetId, const MsgInfo &msg) {
    auto &retransmissions = subscriber->retransmissions;
    if(retransmissions.find(packetId) != retransmissions.end()) {
        throw omnetpp::cRuntimeError("Retransmission with packet Id (%d) already exists", packetId);
    }
    
    RetransmissionInfo info;
    info.msg = msg;
    info.retransmissionEvent = new inet::ClockEvent("RetransmissionTimer");
    info.retransmissionEvent->addPar("isRetransmission");
    info.retransmissionEvent->addPar("packetId") = packetId;
    info.retransmissionEvent->addPar("socket") = subscriber->socket;
    retransmissions[packetId] = info;

    scheduleClockEventAfter(retransmissionInterval, info.retransmissionEvent);
  }


  void MqttServer::insertSubscription(inet::TcpSocket *socket, const std::string &filter, uint8_t qos)
  {
    SessionInfo *subscriber = getClientInfo(socket);
    if(subscriber == nullptr) return;
    
    if(hasWildcardCharacters(filter)) {
      const auto &subscription = wildcardSubscriptions.find(filter);
      if(subscription == wildcardSubscriptions.end()) {
	std::vector<SubscriberInfo> newSubscription;
	newSubscription.push_back(std::make_pair(subscriber, qos));
	wildcardSubscriptions[filter] = newSubscription;
      } else {
	subscription->second.push_back(std::make_pair(subscriber, qos));
      }
    } else {
      const auto &subscription = subscriptions.find(filter);
      if(subscription == subscriptions.end()) {
	std::vector<SubscriberInfo> newSubscription;
	newSubscription.push_back(std::make_pair(subscriber, qos));
	subscriptions[filter] = newSubscription;
      } else {
	subscription->second.push_back(std::make_pair(subscriber, qos));
      }

    }
  }

  void MqttServer::deleteSubscription(inet::TcpSocket *socket, const std::string &filter)
  {
    SessionInfo *subscriber = getClientInfo(socket);
    if(subscriber == nullptr) return;

    if(hasWildcardCharacters(filter)) {
      const auto &subscription = wildcardSubscriptions.find(filter);
      if(subscription == wildcardSubscriptions.end()) return;
      auto& subscribers = subscription->second;
      subscribers.erase(std::remove_if(subscribers.begin(), subscribers.end(),
				       [subscriber](const SubscriberInfo& subInfo) { return subInfo.first == subscriber; }),
			subscribers.end());
        
      if (subscribers.empty()) {
	wildcardSubscriptions.erase(subscription);
      }
    } else {
      const auto &subscription = subscriptions.find(filter);
      if(subscription == subscriptions.end()) return;
      auto& subscribers = subscription->second;
      subscribers.erase(std::remove_if(subscribers.begin(), subscribers.end(),
				       [subscriber](const SubscriberInfo& subInfo) { return subInfo.first == subscriber; }),
			subscribers.end());
        
      if (subscribers.empty()) {
	subscriptions.erase(subscription);
      }

    }
  }

  std::set<uint16_t> MqttServer::getUsedIds(SessionInfo *subscriber) {
    std::set<uint16_t> keys;
    for (auto const& elem : subscriber->retransmissions) {
      keys.insert(elem.first);
    }
    return keys;
  }

  bool MqttServer::hasWildcardCharacters(const std::string &topic) {
    return topic.find("#") != std::string::npos && topic.find("+") != std::string::npos;
  }
  
  MqttServer::~MqttServer()
  {
  }

} /* namespace mqttsn */
