#ifndef MQTT_CONNACK
#define MQTT_CONNACK

#include "MqttWithProperties.h"
#include "../types/Flag.h"
#include <cstdint>
namespace mqtt
{
  class MqttConnAck : public MqttWithProperties {
  public:
    uint8_t connAckFlags;
    uint8_t reasonCode;

  public:
    void setReasonCode(ReasonCode code);
    ReasonCode getReasonCode() const;
  };
}

#endif // MQTT_CONNACK
