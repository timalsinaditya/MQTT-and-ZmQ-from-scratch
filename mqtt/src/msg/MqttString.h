#ifndef MQTT_STRING
#define MQTT_STRING

#include <stdlib.h>
#include <cstdint>
#include <string>

namespace mqtt
{
  class MqttString {
  private:
    uint16_t len;
    std::string str;

  public:
    MqttString(const std::string& str);
    MqttString() { len = 0; }
    
    uint16_t set(const std::string& val);
    std::string get() const;
  };

  typedef MqttString MqttBin;
}

#endif // MQTT_STRING
