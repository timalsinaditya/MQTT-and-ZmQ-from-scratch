#ifndef MQTT_SUBACK
#define MQTT_SUBACK

#include "MqttWithProperties.h"
#include "types/Flag.h"

namespace mqtt
{
  class MqttSubAck : public MqttWithProperties {
  private:
    uint16_t packetIdentifier;

    //payload
    std::vector<SubscribeReasonCode> reasonCodes;
    
  public:
    void setPacketIdentifier(uint16_t id);
    uint16_t getPacketIdentifier() const;

    void addReasonCode(SubscribeReasonCode code);
    std::vector<SubscribeReasonCode> getReasonCodes() const;
  };
}

#endif // MQTT_SUBACK
