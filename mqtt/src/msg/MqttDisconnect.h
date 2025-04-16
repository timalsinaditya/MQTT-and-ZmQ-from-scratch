#ifndef MQTT_DISCONNECT
#define MQTT_DISCONNECT

#include "MqttWithProperties.h"
#include "../types/Flag.h"

namespace mqtt
{
  class MqttDisconnect : public MqttWithProperties {
  private:
    ReasonCode reasonCode;

  public:
    ReasonCode getReasonCode() const;
    void setReasonCode(ReasonCode code);
  };
}

#endif // MQTT_DISCONNECT
