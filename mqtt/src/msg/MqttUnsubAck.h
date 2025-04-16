#ifndef MQTT_UNSUBACK
#define MQTT_UNSUBACK

#include "MqttWithProperties.h"
#include "../types/Flag.h"
#include <cstdint>

namespace mqtt
{
  class MqttUnsubAck : public MqttWithProperties {
  private:
    uint16_t packetIdentifier;

    //payload
    std::vector<UnsubscribeReasonCode> reasonCodes;

  public:
    void setPacketIdentifier(uint16_t id);
    uint16_t getPacketIdentifier() const;

    void addReasonCode(UnsubscribeReasonCode code);
    std::vector<UnsubscribeReasonCode> getReasonCodes() const;
  };
}

#endif // MQTT_UNSUBACK
