#ifndef MQTT_PUBCOMP
#define MQTT_PUBCOMP

#include "MqttWithProperties.h"
#include "../types/Flag.h"
#include <cstdint>

namespace mqtt
{
  class MqttPubComp : public MqttWithProperties {
  private:
    uint16_t packetIdentifier;
    uint8_t reasonCode;

  public:
    void setPacketIdentifier(uint16_t id);
    uint16_t getPacketIdentifier() const;

    void setReasonCode(ReasonCode code);
        ReasonCode getReasonCode() const;
  };
}

#endif // MQTT_PUBCOMP
