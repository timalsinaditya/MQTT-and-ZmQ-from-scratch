#include "MqttDisconnect.h"

namespace mqtt
{
  void MqttDisconnect::setReasonCode(ReasonCode code) {
    reasonCode = code;
  }

  ReasonCode MqttDisconnect::getReasonCode() const {
    return reasonCode;
  }
}
