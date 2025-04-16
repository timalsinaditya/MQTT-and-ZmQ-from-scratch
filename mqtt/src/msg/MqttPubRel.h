#ifndef MQTT_PUBREL
#define MQTT_PUBREL

#include "MqttWithProperties.h"
#include "../types/Flag.h"
#include <cstdint>

namespace mqtt
{
  class MqttPubRel : public MqttWithProperties {
  private:
    uint16_t packetIdentifier;
    ReasonCode reasonCode;

  public:
    void setPacketIdentifier(uint16_t id);
    uint16_t getPacketIdentifier() const;

    void setReasonCode(ReasonCode code);
    ReasonCode getReasonCode() const;
  };
}

#endif // MQTT_PUBREL
