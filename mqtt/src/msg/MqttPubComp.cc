#include "MqttPubComp.h"

namespace mqtt
{
  void MqttPubComp::setPacketIdentifier(uint16_t id) {
    packetIdentifier = id;
  }

  uint16_t MqttPubComp::getPacketIdentifier() const {
    return packetIdentifier;
  }

  void MqttPubComp::setReasonCode(ReasonCode code) {
    reasonCode = (uint8_t)code;
  }

  ReasonCode MqttPubComp::getReasonCode() const {
    return (ReasonCode)reasonCode;
  }
}
