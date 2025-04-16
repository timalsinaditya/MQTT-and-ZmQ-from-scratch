#include "MqttPublish.h"
#include "types/MsgType.h"
#include <cstdint>

namespace mqtt
{
  MqttPublish::MqttPublish() {
    MqttFixed::setMsgType(MsgType::PUBLISH);
    addRemLength(2);   // packetIdentifier
  }
  
  void MqttPublish::setTopicName(const std::string& topic) {
    MqttFixed::addRemLength(topic.length(), topicName.set(topic));
  }

  std::string MqttPublish::getTopicName() const {
    return topicName.get();
  }

  void MqttPublish::setPacketIdentifier(uint16_t id) {
    packetIdentifier = id;
  }

  uint16_t MqttPublish::getPacketIdentifier() const {
    return packetIdentifier;
  }

  void MqttPublish::addPayload(const std::string& msg) {
    MqttFixed::addRemLength(msg.length(), payload.set(msg));
  }

  std::string MqttPublish::getPayload() const {
    return payload.get();
  }
  
}
