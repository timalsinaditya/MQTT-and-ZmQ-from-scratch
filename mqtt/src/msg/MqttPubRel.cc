#include "MqttPubRel.h"

namespace mqtt
{
  void MqttPubRel::setPacketIdentifier(uint16_t id) {
    packetIdentifier = id;
  }

  uint16_t MqttPubRel::getPacketIdentifier() const {
    return packetIdentifier;
  }

  void MqttPubRel::setReasonCode(ReasonCode code) {
    reasonCode = code;
  }

  ReasonCode MqttPubRel::getReasonCode() const {
    return (ReasonCode)reasonCode;
  }
}
