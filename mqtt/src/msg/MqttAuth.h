#ifndef MQTT_AUTH
#define MQTT_AUTH

#include "MqttWithProperties.h"
#include "../types/Flag.h"

namespace mqtt
{
  class MqttAuth : public MqttWithProperties {
  private:
    ReasonCode reasonCode;

  public:
    ReasonCode getReasonCode();
    void setReasonCode(ReasonCode code);
  };
}

#endif // MQTT_AUTH
