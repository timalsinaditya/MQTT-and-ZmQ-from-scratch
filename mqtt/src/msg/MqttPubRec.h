#ifndef MQTT_PUBREC
#define MQTT_PUBREC

#include "MqttWithProperties.h"
#include "../types/Flag.h"
#include <cstdint>

namespace mqtt
{
  class MqttPubRec : public MqttWithProperties {
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

#endif // MQTT_PUBREC
