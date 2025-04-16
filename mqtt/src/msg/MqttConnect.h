#ifndef MQTT_CONNECT
#define MQTT_CONNECT

#include "MqttProperties.h"
#include "MqttWithProperties.h"
#include "MqttString.h"
#include <cstdint>
#include <exception>

namespace mqtt
{
  class MqttConnect : public MqttWithProperties {
  private:
    MqttString protoName;
    uint8_t protoVersion;
    uint16_t keepAlive;

    // payload
    MqttString clientId;
    std::map<Property, std::any> willProperties;
    MqttString willTopic;
    MqttBin willPayload;
    MqttString username;
    MqttBin password;

  public:
    uint8_t connectFlags;
    
  public:
    MqttConnect();
    ~MqttConnect() {};

    std::string getProtoName() const;
    uint8_t getProtoVersion() const;

    void setKeepAlive(uint16_t ka);
    uint16_t getKeepAlive() const;

    void addWill(const std::string &topic, const std::string &msg);
    std::string getWillTopic() const;
    std::string getWillPayload() const;

    void setUsername(const std::string &name);
    std::string getUsername() const;

    void setPassword(const std::string &name);
    std::string getPassword() const;

    void addWillProperty(Property property, std::any& val);
    std::any getWillProperty(Property property) const;

    void setClientId(std::string &id);
    std::string getClientId() const;

  };
}

#endif // MQTT_CONNECT
