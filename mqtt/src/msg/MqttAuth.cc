#include "MqttAuth.h"

namespace mqtt
{
  void MqttAuth::setReasonCode(ReasonCode code) {
    reasonCode = code;
  }

  ReasonCode MqttAuth::getReasonCode() {
    return reasonCode;
  }
}
