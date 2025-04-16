#ifndef MQTT_SUBSCRIBE
#define MQTT_SUBSCRIBE

#include "MqttWithProperties.h"
#include "MqttString.h"
#include "../types/Flag.h"
#include <cstdint>
#include <vector>

namespace mqtt
{
  class MqttSubscribe : public MqttWithProperties {
  private:
    uint16_t packetIdentifier;
    std::vector<std::pair<MqttString, uint8_t>> topicFilters;

  public:
    void setPacketIdentifier(uint16_t id);
    uint16_t getPacketIdentifier() const;

    void addTopicFilter(std::string &filter, uint8_t options);
    std::vector<std::pair<MqttString, uint8_t>> getTopicFilters() const;
  };
}

#endif // MQTT_SUBSCRIBE
