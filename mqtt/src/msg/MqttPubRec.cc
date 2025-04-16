#include "MqttPubRec.h"

namespace mqtt
{
  void MqttPubRec::setPacketIdentifier(uint16_t id) {
    packetIdentifier = id;
  }

  uint16_t MqttPubRec::getPacketIdentifier() const {
    return packetIdentifier;
  }

  void MqttPubRec::setReasonCode(ReasonCode code) {
    reasonCode = code;
  }

  ReasonCode MqttPubRec::getReasonCode() const {
    return (ReasonCode)reasonCode;
  }
}
