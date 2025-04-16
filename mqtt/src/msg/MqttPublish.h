#ifndef MQTT_PUBLISH_H
#define MQTT_PUBLISH_H

#include "MqttWithProperties.h"
#include "MqttString.h"
#include <cstdint>
#include <string>

namespace mqtt
{
  class MqttPublish : public MqttWithProperties {
  private:
    MqttString topicName;
    uint16_t packetIdentifier;

    //payload
    MqttString payload;

  public:
    MqttPublish();
    
    void setTopicName(const std::string &topic);
    std::string getTopicName() const;

    void setPacketIdentifier(uint16_t id);
    uint16_t getPacketIdentifier() const;

    void addPayload(const std::string& msg);
    std::string getPayload() const;
  };
}

#endif // MQTT_PUBLISH_H
