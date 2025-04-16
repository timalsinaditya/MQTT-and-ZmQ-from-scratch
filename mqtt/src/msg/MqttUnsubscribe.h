#ifndef MQTT_UNSUBSCRIBE
#define MQTT_UNSUBSCRIBE

#include "MqttWithProperties.h"
#include "../types/Flag.h"
#include <cstdint>

namespace mqtt
{
  class MqttUnsubscribe : public MqttWithProperties {
  private:
    uint16_t packetIdentifier;

    //payload
    std::vector<MqttString> topicFilters;

  public:
    void setPacketIdentifier(uint16_t id);
    uint16_t getPacketIdentifier() const;

    void addTopicFilter(std::string &filter);
    const std::vector<MqttString>& getTopicFilters() const;
  };
}

#endif // MQTT_UNSUBSCRIBE
