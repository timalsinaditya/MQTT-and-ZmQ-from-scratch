#include "MqttPubAck.h"

namespace mqtt
{
  void MqttPubAck::setPacketIdentifier(uint16_t id) {
    packetIdentifier = id;
  }

  uint16_t MqttPubAck::getPacketIdentifier() const {
    return packetIdentifier;
  }

  void MqttPubAck::setReasonCode(ReasonCode code) {
    reasonCode = code;
  }

  ReasonCode MqttPubAck::getReasonCode() const {
    return (ReasonCode)reasonCode;
  }
}
