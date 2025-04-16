#include "MqttClient.h"
#include "msg/MqttFixed.h"
#include "msg/MqttConnect.h"
#include "msg/MqttConnAck.h"
#include "msg/MqttDisconnect.h"
#include "msg/MqttPublish.h"
#include "msg/MqttPubAck.h"
#include "msg/MqttPubRec.h"
#include "msg/MqttPubRel.h"
#include "msg/MqttPubComp.h"
#include "msg/MqttSubscribe.h"
#include "msg/MqttSubAck.h"
#include "msg/MqttUnsubscribe.h"
#include "msg/MqttUnsubAck.h"
#include "msg/MqttPingReq.h"

#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/packet/chunk/Chunk.h"

using std::endl;

namespace mqtt
{
  Define_Module(MqttClient);
  Register_Class(MqttClient);
  
  MqttClient::MqttClient() {
    connectEvent = new inet::ClockEvent("connectTimer");
    publishEvent = new inet::ClockEvent("publishTimer");
    subscriptionEvent = new inet::ClockEvent("subscriptionTimer");
    unsubscriptionEvent = new inet::ClockEvent("unsubscriptionTimer");
    pingEvent = new inet::ClockEvent("pingEvent");

    toSubscribe.push_back("temp");
    toSubscribe.push_back("hello");
    toSubscribe.push_back("other");

    publishTopics = toSubscribe;

    lastPacketId = 0;
  }

  MqttClient::~MqttClient() {
    cancelAndDelete(connectEvent);
    cancelAndDelete(publishEvent);
    cancelAndDelete(subscriptionEvent);
    cancelAndDelete(unsubscriptionEvent);
    cancelAndDelete(pingEvent);
    clearRetransmissions();
  }
  
  void MqttClient::init(int stage) {
    if(stage == inet::INITSTAGE_APPLICATION_LAYER) {
        serverAddress = inet::L3AddressResolver().resolve(par("serverAddress"));
        configureSocket();
        localSocket.connect(serverAddress, serverPort);
        serverToClientDelay.setName("Server to Client latency");
        publishPacketSize.setName("Publish Packet Size");
        subscribePacketSize.setName("Subscribe Packet Size");
        connectPacketSize.setName("Connect Packet Size");
        pingPacketSize.setName("Ping Packet Size");
    }

    serverPort = par("serverPort");

    keepAlive = par("keepAlive");
    cleanSession = par("cleanSession");
    reconnectInterval = par("reconnectInterval");
    publishInterval = par("publishInterval");
    subscriptionInterval = par("subscriptionInterval");
    unsubscriptionInterval = par("unsubscriptionInterval");
    maxRetransmissions = par("maxRetransmissions");
    retransmissionInterval = par("retransmissionInterval");
    publishDataSize = par("publishDataSize");
    packetBER = par("packetBER");

    isConnected = false;

    pingEventInterval = 0.75 * keepAlive;
    clientId = getClientId();
  }

  void MqttClient::finish() {
    // TODO : handle sim results
    // recordScalar("packets_sent", packetsSent);
    // recordScalar("packets_received", packetsReceived);
    // recordScalar("bytes_sent", bytesSent);
    // recordScalar("bytes_received", bytesReceived);

      recordScalar("Packets Sent", packetsSent);
      recordScalar("Packets Received", packetsReceived);
      recordScalar("Retransmissions", numRetransmission);
      recordScalar("Erroneous Packets", erroneousPackets);

      recordScalar("Publish Packets Sent", publishPacketSent);
      recordScalar("Subscribe Packets Sent", subscribePacketSent);
      recordScalar("Connect Packets Sent", connectPacketSent);
      recordScalar("Ping Packets Sent", pingPacketSent);
    
    MqttApp::finish();
  }

  void MqttClient::handleStartOperation(inet::LifecycleOperation* operation) {

  }
  
  void MqttClient::handleStopOperation(inet::LifecycleOperation* operation) {
    // TODO : also clear unacknowledged list
    clearRetransmissions();
    clearReceivedQos2Messages();

    MqttApp::localSocket.close();
  }
  
  void MqttClient::handleCrashOperation(inet::LifecycleOperation* operation) {
    clearRetransmissions();
    clearReceivedQos2Messages();
    
    MqttApp::localSocket.destroy();
  }

  void MqttClient::socketEstablished(inet::TcpSocket *socket) {
    // TCP connection established, now send CONNECT message
    scheduleClockEventAfter(0, connectEvent);
  }

  void MqttClient::socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *info) {

  }

  void MqttClient::socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent) {
    processPacket(packet, socket);
  }

  void MqttClient::socketPeerClosed(inet::TcpSocket *socket) {
    EV_INFO << "TCP connection closed by peer\n";
    isConnected = false;
    socket->close();
  }

  void MqttClient::socketClosed(inet::TcpSocket *socket) {
    EV_INFO << "TCP connection closed\n";
    isConnected = false;
  }

  void MqttClient::socketFailure(inet::TcpSocket *socket, int code) {
    EV_WARN << "TCP connection failure, code " << code << "\n";
    isConnected = false;
    socket->close();
  }

  std::string MqttClient::padData(const std::string& data, uint32_t requiredSize) {
      std::string paddedData = data;
      while (paddedData.size() < requiredSize) {
          paddedData += data;
      }
      return paddedData.substr(0, requiredSize);
  }

  std::string MqttClient::generatePublishData() {
      uint32_t dataSize = publishDataSize;
      std::ostringstream dataStream;
      std::srand(std::time(0));

      float temperature = uniform(-20.0, 50.0);
      float speed = uniform(0.0, 200.0);
      float locationLat = uniform(-90.0, 90.0);
      float locationLon = uniform(-180.0, 180.0);
      float fuelLevel = uniform(0.0, 100.0);
      int brakeStatus = intuniform(0, 1);

      dataStream << "{"
                 << "\"temperature\":" << temperature << ","
                 << "\"speed\":" << speed << ","
                 << "\"location\": {"
                 << "\"lat\":" << locationLat << ","
                 << "\"lon\":" << locationLon << "},"
                 << "\"fuelLevel\":" << fuelLevel << ","
                 << "\"brakeStatus\":" << brakeStatus
                 << "}";
      std::string data = dataStream.str();

      if (data.size() < dataSize) {
          return padData(data, dataSize);
      } else if (data.size() > dataSize) {
          return data.substr(0, dataSize);
      }

      return data;
  }

  void MqttClient::handleMessageWhenUp(omnetpp::cMessage *msg) {
    if (msg == connectEvent) {
      handleConnectEvent();
    } else if (msg == pingEvent) {
      handlePingEvent();
    } else if (msg == subscriptionEvent) {
      handleSubscriptionEvent();
    } else if (msg == unsubscriptionEvent) {
      handleUnsubscriptionEvent();
    } else if (msg == publishEvent) {
      handlePublishEvent();
    } else if (msg->hasPar("isRetransmission")) {
	uint16_t packetId = msg->par("packetId");
	if(retransmissions.find(packetId) != retransmissions.end()) {
	  handleRetransmission(packetId);
	}
      }
    else {
      MqttApp::localSocket.processMessage(msg);
    }
  }

  void MqttClient::handleConnectEvent() {
    if (!isConnected) {
      sendConnect(keepAlive);
    }
  }

  void MqttClient::handlePingEvent() {
      sendPingReq();
      scheduleClockEventAfter(pingEventInterval, pingEvent);
  }

  void MqttClient::handleSubscriptionEvent() {
      if(toSubscribe.size() > 0) {
          uint16_t packetId = getNextPacketId(getUsedPacketIds(), lastPacketId);
          sendSubscribe(packetId, toSubscribe[0]);

          MsgInfo info;
          info.type = SUBSCRIBE;
          info.topic = toSubscribe[0];
          info.packetId = packetId;
          scheduleRetransmission(packetId, info);
          subscribedTo.push_back(toSubscribe[0]);
          toSubscribe.erase(toSubscribe.begin());
      }
  }

  void MqttClient::handleUnsubscriptionEvent() {
    if(subscribedTo.size() > 0) {
        uint16_t packetId = getNextPacketId(getUsedPacketIds(), lastPacketId);
        MsgInfo info;
        info.type = UNSUBSCRIBE;
        info.topic = subscribedTo[0];
        info.packetId = packetId;

        sendUnsubscribe(packetId, toSubscribe[0]);
        scheduleRetransmission(packetId, info);
        subscribedTo.erase(subscribedTo.begin());
     }
  }

  void MqttClient::handlePublishEvent() {
      uint16_t packetId = getNextPacketId(getUsedPacketIds(), lastPacketId);
      MsgInfo info;
      info.type = PUBLISH;
      info.packetId = packetId;
      info.data = generatePublishData();
      info.topic = publishTopics[intuniform(0, publishTopics.size()-1)];
      info.qos = 0;

      sendPublish(info.packetId, info.qos, false, info.topic, info.data);
      scheduleClockEventAfter(publishInterval, publishEvent);
      // scheduleRetransmission(packetId, info);
  }

  void MqttClient::handleRetransmission(uint16_t packetId) {
    if (retransmissions.find(packetId) == retransmissions.end()) {
      return;
    }

    numRetransmission++;
    RetransmissionInfo &info = retransmissions[packetId];
    if (info.retransmissionCounter >= maxRetransmissions) {
      EV << "Max retransmissions reached for packet " << packetId << ". Giving up." << endl;
      retransmissions.erase(packetId);
      return;
    }
    info.retransmissionCounter++;

    switch (info.msg.type) {
    case MsgType::PUBLISH:
      sendPublish(packetId, info.msg.qos, true, info.msg.topic, info.msg.data);
      break;
    case MsgType::PUBREC:
      sendPubRec(packetId, info.msg.reasonCode);
      break;
    case MsgType::PUBREL:
      sendPubRel(packetId, info.msg.reasonCode);
      break;
    case MsgType::SUBSCRIBE:
      sendSubscribe(packetId, info.msg.topic, info.msg.qos);
      break;
    case MsgType::UNSUBSCRIBE:
      sendUnsubscribe(packetId, info.msg.topic);
      break;
    default:
      EV_ERROR << "Unexpected message type for retransmission: " << (int)info.msg.type << endl;
      return;
    }

    scheduleClockEventAfter(retransmissionInterval, info.retransmissionEvent);
  }  

  void MqttClient::processPacket(inet::Packet *pk, inet::TcpSocket *socket) {
      // Check if the packet contains a SequenceChunk
      const auto& chunk = pk->peekDataAt<inet::Chunk>(inet::B(0), pk->getTotalLength());

      const inet::SequenceChunk* sequenceChunk = dynamic_cast<const inet::SequenceChunk*>(chunk.get());

      if (sequenceChunk) {
          // The packet contains a SequenceChunk, process its chunks
          EV << "Processing SequenceChunk" << endl;

          // Get the deque of chunks from the SequenceChunk
          const std::deque<inet::Ptr<const inet::Chunk>>& chunks = sequenceChunk->getChunks();

          for (const auto& subchunk : chunks) {
              // Check if the subchunk is of type MqttFixed
              const auto* header = dynamic_cast<const MqttFixed*>(subchunk.get());

              if (header) {
                  // Check packet integrity and process by message type
                  MqttApp::checkPacketIntegrity((inet::B) subchunk->getChunkLength(), (inet::B) header->getRemLength());

                  MsgType type = header->getMsgType();
                  inet::Packet* newPacket = new inet::Packet();
                  newPacket->insertAtBack(subchunk);

                  if (newPacket->hasBitError()) {
                      erroneousPackets++;
                      delete newPacket;
                      continue;
                  }
                  processPacketByMsgType(newPacket, type);
                  delete newPacket;

                  packetsReceived++;
              } else {
                  EV_WARN << "Subchunk is not an MqttFixed header!" << endl;
              }
          }
      }
      else {
          if (pk->hasBitError()) {
              erroneousPackets++;
              delete pk;
              return;
          }

          // Fallback for regular processing if it's not a SequenceChunk
          auto chunk = pk->peekData<inet::Chunk>();

          // Check if the chunk is a SliceChunk
          auto sliceChunk = inet::dynamicPtrCast<const inet::SliceChunk>(chunk);
          inet::Ptr<const MqttFixed> header;

          if (sliceChunk) {
              // Extract the original chunk from the SliceChunk
              auto originalChunk = sliceChunk->getChunk();
              header = inet::dynamicPtrCast<const MqttFixed>(originalChunk);
          } else {
              // Directly try to cast the chunk if it's not a SliceChunk
              header = inet::dynamicPtrCast<const MqttFixed>(chunk);
          }

          // Ensure header is valid before proceeding
          if (!header) {
              EV_WARN << "Failed to extract valid MqttFixed header!" << endl;
              delete pk;
              return;
          }

          MqttApp::checkPacketIntegrity((inet::B)pk->getByteLength(), (inet::B)header->getRemLength());

          EV << "Packet Received by Client" << endl;

          MsgType type = header->getMsgType();
          processPacketByMsgType(pk, type);

          packetsReceived++;
      }


      delete pk;
  }


  void MqttClient::processPacketByMsgType(inet::Packet *pk, MsgType type) {
    switch(type) {
    case MsgType::CONNACK:
      processConnAck(pk);
      break;

    case MsgType::PINGRESP:
      processPingResp();
      break;
      
    case MsgType::DISCONNECT:
      processDisconnect(pk);
      break;

    case MsgType::PUBLISH:
      processPublish(pk);
      break;

    case MsgType::PUBACK:
      processPubAck(pk);
      break;

    case MsgType::PUBREC:
      processPubRec(pk);
      break;

    case MsgType::PUBREL:
      processPubRel(pk);
      break;
      
    case MsgType::PUBCOMP:
      processPubComp(pk);
      break;      

    case MsgType::SUBACK:
      processSubAck(pk);
      break;

    case MsgType::UNSUBACK:
      processUnsubAck(pk);
      break;

    default:
      break;
    }
  }

  void MqttClient::processConnAck(inet::Packet *pk) {
    const auto &payload = pk->peekData<MqttConnAck>();

    if((uint8_t)payload->getReasonCode() != (uint8_t)ConnectReasonCode::SUCCESS) {
      // TODO : Reschedule connection
      scheduleClockEventAfter(reconnectInterval, connectEvent);      
      return;
    }

    isConnected = true;
    EV << "Client connected to: " << serverAddress << ":" << serverPort << endl;

    scheduleClockEventAfter(publishInterval, publishEvent);
    scheduleClockEventAfter(subscriptionInterval, subscriptionEvent);
    scheduleClockEventAfter(pingEventInterval, pingEvent);
  }

  void MqttClient::processPingResp() {
    EV << "Received ping response from server: " << serverAddress << ":" << serverPort << endl;
//    scheduleClockEventAfter(pingEventInterval, pingEvent);
  }

  void MqttClient::processDisconnect(inet::Packet *pk) {
    EV << "Client received disconnect message from server. Attempting to reconnect" << endl;
    isConnected = false;
    clearRetransmissions();
    clearReceivedQos2Messages();
    scheduleClockEventAfter(reconnectInterval, connectEvent);    
    // TODO : schedule connect
  }

  void MqttClient::processPublish(inet::Packet *pk) {
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
    // const auto &payload = pk->peekData<MqttPublish>();
    uint16_t packetId = payload->getPacketIdentifier();
    if(receivedQos2Messages.find(packetId) != receivedQos2Messages.end()) {
      sendPubRec(packetId, (uint8_t)PubAckReasonCode::PACKET_ID_IN_USE);
      return;
    }

    double creationTime = payload->getCreationTime();
    double delay = omnetpp::simTime().dbl() - creationTime;
    serverToClientDelay.record(delay);

    uint8_t qos = payload->getFlag(Flag::QOS, payload->flags);
    std::string data = payload->getPayload();
    std::string topic = payload->getTopicName();

    if(qos < 2) {
      // print message
      // TODO : handle metrics
      if(qos == 1) {
        sendPubAck(packetId, (uint8_t)PubAckReasonCode::SUCCESS);
      }
    }
    else if(qos == 2) {
      MsgInfo info;
      info.type = MsgType::PUBLISH;
      info.packetId = packetId;
      info.topic = topic;
      info.data = data;
      info.qos = 2;
      
      if (receivedQos2Messages.find(packetId) == receivedQos2Messages.end()) {
        receivedQos2Messages[packetId] = info;
      } else {
	throw omnetpp::cRuntimeError("Packet ID already in use");
      }
      
      sendPubRec(packetId, (uint8_t)PubAckReasonCode::SUCCESS);
      scheduleRetransmission(packetId, {MsgType::PUBREC, packetId, 0, "", "", 0, 0});
    }
  }

  void MqttClient::processAckWithId(MsgType type, uint16_t packetId, uint8_t reasonCode) {
    uint8_t successCode = 0;
    switch(type) {
    case PUBACK: successCode = (uint8_t) PubAckReasonCode::SUCCESS; break;
    case PUBREC: successCode = (uint8_t) PubAckReasonCode::SUCCESS; break;
    case PUBREL: successCode = (uint8_t) PubRelReasonCode::SUCCESS; break;
    case PUBCOMP: successCode = (uint8_t) PubRelReasonCode::SUCCESS; break;      
    default: break;
    }

    if(retransmissions.find(packetId) == retransmissions.end()) {
      EV_WARN << "Ignoring Unexpected Pulish Acknowledgement: ID = " << packetId << endl;
      return;
    }
    // TODO : remove from the retransmissions list
    cancelAndDelete(retransmissions[packetId].retransmissionEvent);
    retransmissions.erase(packetId);
    
    if(reasonCode != successCode) {
      throw omnetpp::cRuntimeError("Server Returned Error: %d", reasonCode);
    }
  }

  void MqttClient::processPubAck(inet::Packet *pk) {
    const auto &payload = pk->peekData<MqttPubAck>();
    uint16_t packetId = payload->getPacketIdentifier();
    uint8_t reasonCode = (uint8_t)payload->getReasonCode();
    processAckWithId(MsgType::PUBACK, packetId, reasonCode);

    scheduleClockEventAfter(publishInterval, publishEvent);
  }

  void MqttClient::processPubRec(inet::Packet *pk) {
    const auto &payload = pk->peekData<MqttPubRec>();
    uint16_t packetId = payload->getPacketIdentifier();
    uint8_t reasonCode = (uint8_t)payload->getReasonCode();
    processAckWithId(MsgType::PUBREC, packetId, reasonCode);

    sendPubRel(packetId, reasonCode);
    scheduleRetransmission(packetId, {MsgType::PUBREL, packetId, 0, "", "", 0, 0});    
    // TODO : schedule retransmission of pub rel    
  }

  void MqttClient::processPubRel(inet::Packet *pk) {
    const auto &payload = pk->peekData<MqttPubRel>();
    uint16_t packetId = payload->getPacketIdentifier();
    uint8_t reasonCode = (uint8_t)payload->getReasonCode();
    processAckWithId(MsgType::PUBREL, packetId, reasonCode);

    // TODO : print the store message with the packetId
    if (receivedQos2Messages.find(packetId) != receivedQos2Messages.end()) {
      const auto& msg = receivedQos2Messages[packetId];
      EV << "Processing QoS 2 message: Topic = " << msg.topic << ", Data = " << msg.data << endl;
      receivedQos2Messages.erase(packetId);
    } else {
      EV_WARN << "Unexpected PubRel with ID : " << packetId << endl; 
      sendPubComp(packetId, (uint8_t)PubRelReasonCode::PACKET_ID_NOT_FOUND);
      return;
    }
    
    sendPubComp(packetId, (uint8_t)PubRelReasonCode::SUCCESS);
  }

  void MqttClient::processPubComp(inet::Packet *pk) {
    const auto &payload = pk->peekData<MqttPubComp>();
    uint16_t packetId = payload->getPacketIdentifier();
    uint8_t reasonCode = (uint8_t)payload->getReasonCode();
    processAckWithId(MsgType::PUBCOMP, packetId, reasonCode);

    scheduleClockEventAfter(publishInterval, publishEvent);
  }

  void MqttClient::processSubAck(inet::Packet *pk) {
    const auto &payload = pk->peekData<MqttSubAck>();
    uint16_t packetId = payload->getPacketIdentifier();
    uint8_t reasonCode = (uint8_t)payload->getReasonCodes()[0]; // only one subscription per message ?

    RetransmissionInfo lastSubMsg;
    if (auto it = retransmissions.find(packetId); it != retransmissions.end()) {
        cancelAndDelete(it->second.retransmissionEvent);
        lastSubMsg = std::move(it->second);
        retransmissions.erase(it);
    }

    if(reasonCode <= (uint8_t)SubscribeReasonCode::GRANTED_QOS2) {
      // TODO : schedule another subscription
      scheduleClockEventAfter(subscriptionInterval, subscriptionEvent);
      return; // successfull
    }

    if(reasonCode == (uint8_t)SubscribeReasonCode::PACKET_ID_IN_USE) {
      uint16_t newPacketId = getNextPacketId(getUsedPacketIds(), lastPacketId);
      sendSubscribe(newPacketId, lastSubMsg.msg.topic, lastSubMsg.msg.subOpts);
      scheduleRetransmission(newPacketId, lastSubMsg.msg);
      // TODO : cancel the current retransmission
      // TODO : send the last subscribe with new packet id
    }
  }

  void MqttClient::processUnsubAck(inet::Packet *pk) {
    const auto &payload = pk->peekData<MqttUnsubAck>();
    uint16_t packetId = payload->getPacketIdentifier();
    uint8_t reasonCode = (uint8_t)payload->getReasonCodes()[0];

    RetransmissionInfo lastUnsubMsg;
    if (auto it = retransmissions.find(packetId); it != retransmissions.end()) {
        cancelAndDelete(it->second.retransmissionEvent);
        lastUnsubMsg = std::move(it->second);
        retransmissions.erase(it);
    }

    if(reasonCode == (uint8_t)UnsubscribeReasonCode::PACKET_ID_IN_USE) {
      uint16_t newPacketId = getNextPacketId(getUsedPacketIds(), lastPacketId);
      lastUnsubMsg.msg.packetId = newPacketId;
      sendUnsubscribe(newPacketId, lastUnsubMsg.msg.topic);

      scheduleRetransmission(newPacketId, lastUnsubMsg.msg);
    }
  }

  void MqttClient::scheduleRetransmission(uint16_t packetId, const MsgInfo &msg) {
    if(retransmissions.find(packetId) != retransmissions.end()) {
        throw omnetpp::cRuntimeError("Retransmission with packet Id (%d) already exists", packetId);
    }
    
    RetransmissionInfo info;
    info.msg = msg;
    info.retransmissionEvent = new inet::ClockEvent("RetransmissionTimer");
    info.retransmissionEvent->addPar("isRetransmission");
    info.retransmissionEvent->addPar("packetId") = packetId;
    retransmissions[packetId] = info;

    scheduleClockEventAfter(retransmissionInterval, info.retransmissionEvent);
  }

  void MqttClient::clearRetransmissions() {
    for (auto &pair : retransmissions) {
      cancelAndDelete(pair.second.retransmissionEvent);
    }
    retransmissions.clear();
  }

  void MqttClient::clearReceivedQos2Messages() {
    receivedQos2Messages.clear();
  }
  
  void MqttClient::sendConnect(uint16_t keepAlive, uint8_t willQos,
			       const std::string &willTopic, const std::string &willMsg,
			       const std::string &username, const std::string &password) {
    const auto &payload = inet::makeShared<MqttConnect>();
    payload->setMsgType(MsgType::CONNECT);
    payload->setKeepAlive(keepAlive);
    payload->setClientId(clientId);
    if(!willTopic.empty()) payload->addWill(willTopic, willMsg);
    if(!username.empty()) payload->setUsername(username);
    if(!password.empty()) payload->setUsername(password);
    payload->setChunkLength(inet::B(payload->getRemLength() + 2));
    
    connectPacketSize.record(payload->getRemLength() + 2);
    connectPacketSent++;

    inet::Packet *packet = new inet::Packet("ConnectPacket");
    packet->insertAtBack(payload);
    MqttApp::corruptPacket(packet, MqttApp::packetBER);
    packetsSent++;

    MqttApp::localSocket.send(packet);
  }

  void MqttClient::sendPublish(uint16_t packetId, uint8_t qos, bool dupFlag,
			       const std::string &topic, const std::string &data) {
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
    publishPacketSize.record(payload->getRemLength() + 2);
    publishPacketSent++;

    inet::Packet *packet = new inet::Packet("PublishPacket");
    packet->insertAtBack(payload);
    corruptPacket(packet, packetBER);
    packetsSent++;

    MqttApp::localSocket.send(packet);

    if (qos > 0) {
      MsgInfo msgInfo = {MsgType::PUBLISH, packetId, 0, topic, data, qos, 0};
      scheduleRetransmission(packetId, msgInfo);
    }
  }

  void MqttClient::sendPacketIdWithReason(MsgType type, uint16_t packetId, uint8_t reasonCode) {
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
    
    MqttApp::localSocket.send(packet);
  }
  
  void MqttClient::sendPubAck(uint16_t packetId, uint8_t reasonCode) {
    sendPacketIdWithReason(MsgType::PUBACK, packetId, reasonCode);
  }

  void MqttClient::sendPubRec(uint16_t packetId, uint8_t reasonCode) {
    sendPacketIdWithReason(MsgType::PUBREC, packetId, reasonCode);
  }

  void MqttClient::sendPubRel(uint16_t packetId, uint8_t reasonCode) {
    sendPacketIdWithReason(MsgType::PUBREL, packetId, reasonCode);
  }

  void MqttClient::sendPubComp(uint16_t packetId, uint8_t reasonCode) {
    sendPacketIdWithReason(MsgType::PUBCOMP, packetId, reasonCode);
  }

  void MqttClient::sendPingReq() {
    const auto &payload = inet::makeShared<MqttPingReq>();
    payload->setMsgType(MsgType::PINGREQ);
    payload->setChunkLength(inet::B(payload->getRemLength() + 2));

    pingPacketSize.record(payload->getRemLength() + 2);
    pingPacketSent++;

    inet::Packet *packet = new inet::Packet("PingReqPacket");
    packet->insertAtBack(payload);
    corruptPacket(packet, packetBER);
    packetsSent++;

    MqttApp::localSocket.send(packet);
  }

  void MqttClient::sendSubscribe(uint16_t packetId, std::string &topicFilter,
				 uint8_t options) {
    const auto &payload = inet::makeShared<MqttSubscribe>();
    payload->setMsgType(MsgType::SUBSCRIBE);
    payload->setPacketIdentifier(packetId);
    payload->addTopicFilter(topicFilter, options);
    payload->setChunkLength(inet::B(payload->getRemLength() + 2));

    subscribePacketSize.record(payload->getRemLength() + 2);
    subscribePacketSent++;

    inet::Packet *packet = new inet::Packet("SubscribePacket");
    packet->insertAtBack(payload);
    corruptPacket(packet, packetBER);
    packetsSent++;

    MqttApp::localSocket.send(packet);
  }

  void MqttClient::sendUnsubscribe(uint16_t packetId, std::string &topicFilter) {
    const auto &payload = inet::makeShared<MqttUnsubscribe>();
    payload->setMsgType(MsgType::UNSUBSCRIBE);
    payload->setPacketIdentifier(packetId);
    payload->addTopicFilter(topicFilter);
    payload->setChunkLength(inet::B(payload->getRemLength() + 2));

    inet::Packet *packet = new inet::Packet("UnSubscribePacket");
    packet->insertAtBack(payload);
    corruptPacket(packet, packetBER);

    MqttApp::localSocket.send(packet);
    packetsSent++;
  }


  std::string MqttClient::getClientId() {
    uint8_t minLen = 2;
    uint8_t maxLen = 23;
    uint8_t len = intuniform(minLen, maxLen);

    std::string allowed = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string clientId;

    for (uint8_t i = 0; i < len; ++i) {
      clientId += allowed[intuniform(0, allowed.length() - 1)];
    }

    return clientId;
  }

  std::set<uint16_t> MqttClient::getUsedPacketIds() {
    std::set<uint16_t> keys;
    for (auto const& elem : retransmissions) {
      keys.insert(elem.first);
    }
    return keys;
  }
}
