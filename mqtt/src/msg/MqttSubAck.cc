#include "MqttSubAck.h"

namespace mqtt
{
  void MqttSubAck::setPacketIdentifier(uint16_t id) {
    packetIdentifier = id;
  }

  uint16_t MqttSubAck::getPacketIdentifier() const {
    return packetIdentifier;
  }

  void MqttSubAck::addReasonCode(SubscribeReasonCode code) {
    reasonCodes.push_back(code);
    addRemLength(1);
  }

  std::vector<SubscribeReasonCode> MqttSubAck::getReasonCodes() const {
    return reasonCodes;
  }
}
