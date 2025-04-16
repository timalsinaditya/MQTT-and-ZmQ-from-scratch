#include "MqttUnsubAck.h"

namespace mqtt
{
  void MqttUnsubAck::setPacketIdentifier(uint16_t id) {
    packetIdentifier = id;
  }

  uint16_t MqttUnsubAck::getPacketIdentifier() const {
    return packetIdentifier;
  }

  void MqttUnsubAck::addReasonCode(UnsubscribeReasonCode code) {
    reasonCodes.push_back(code);
    addRemLength(1);
  }

  std::vector<UnsubscribeReasonCode> MqttUnsubAck::getReasonCodes() const {
    return reasonCodes;
  }

}
