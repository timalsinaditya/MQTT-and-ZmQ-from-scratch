#include "MqttConnect.h"
#include "../types/MsgType.h"
#include "MqttProperties.h"
#include "MqttWithProperties.h"
#include <cstdint>
#include <string>

namespace mqtt
{
  MqttConnect::MqttConnect() {
    MqttFixed::setMsgType(MsgType::CONNECT);
    MqttFixed::addRemLength(10);
    std::string name ("MQTT", 4);
    protoName.set(name);
    protoVersion = 0x05;
  }

  std::string MqttConnect::getProtoName() const {
    return protoName.get();
  }

  uint8_t MqttConnect::getProtoVersion() const {
    return protoVersion;
  }

  void MqttConnect::setKeepAlive(uint16_t ka) {
    keepAlive = ka;
  }

  uint16_t MqttConnect::getKeepAlive() const {
    return keepAlive;
  }

  void MqttConnect::addWill(const std::string& topic, const std::string& msg) {
    MqttFixed::addRemLength(topic.length(), willTopic.set(topic));
    MqttFixed::addRemLength(topic.length(), willPayload.set(msg));

    MqttFixed::setFlag(1, (uint8_t)ConnectFlag::WILL_FLAG, connectFlags);
  }

  std::string MqttConnect::getWillTopic() const {
    return willTopic.get();
  }

  std::string MqttConnect::getWillPayload()  const {
    return willPayload.get();
  }

  void MqttConnect::setUsername(const std::string& name) {
    MqttFixed::addRemLength(name.length(), username.set(name));
    MqttFixed::setFlag(1, ConnectFlag::USERNAME_FLAG, connectFlags);    
  }  
  
  std::string MqttConnect::getUsername() const {
    return username.get();
  }

  void MqttConnect::setPassword(const std::string& name) {
    MqttFixed::addRemLength(name.length(), password.set(name));
    MqttFixed::setFlag(1, ConnectFlag::PASSWORD_FLAG, connectFlags);    
  }

  std::string MqttConnect::getPassword() const {
    return password.get();
  }

  void MqttConnect::addWillProperty(Property property, std::any& val) {
    MqttWithProperties::WriteProperty(willProperties, property, val);
  }

  std::any MqttConnect::getWillProperty(Property property) const {
    return MqttWithProperties::GetProperty(willProperties, property);
  }

  void MqttConnect::setClientId(std::string& id) {
    MqttFixed::addRemLength(id.length(), clientId.set(id));
  }  
  
  std::string MqttConnect::getClientId() const {
    return clientId.get();
  }

}
