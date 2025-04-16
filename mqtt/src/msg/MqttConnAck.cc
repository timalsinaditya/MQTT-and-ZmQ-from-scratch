#include "MqttConnAck.h"

namespace mqtt
{
  void MqttConnAck::setReasonCode(ReasonCode code) {
    reasonCode = (uint8_t)code;
  }

  ReasonCode MqttConnAck::getReasonCode() const {
    return (ReasonCode)reasonCode;
  }
}
