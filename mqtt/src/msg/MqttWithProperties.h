#ifndef MQTT_WITH_PROPERTIES
#define MQTT_WITH_PROPERTIES

#include "MqttWithPayload.h"
#include "MqttProperties.h"
#include "VarInt.h"
#include "MqttString.h"
#include <map>

namespace mqtt
{
  class MqttWithProperties : public MqttWithPayload {
  private:
    VarInt propertyLen;
    std::map<Property, std::any> properties;

  public:
    void WriteProperty(std::map<Property, std::any> &ref, Property type, std::any& val);
    std::any GetProperty(const std::map<Property, std::any> &ref, Property type) const;
  };
}

#endif // MQTT_WITH_PROPERTIES
