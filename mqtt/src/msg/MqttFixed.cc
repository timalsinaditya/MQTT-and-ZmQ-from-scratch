#include "MqttFixed.h"
#include "MqttString.h"
#include "MqttProperties.h"
#include <any>
#include <cstdint>
#include <omnetpp.h>

namespace mqtt {

  Register_Class(MqttFixed);

  void MqttFixed::setFlag(uint8_t pos, uint8_t value, uint8_t &flags) {
    flags = (flags & ~(0b11 << pos)) | (value << pos);
  }

  uint8_t MqttFixed::getFlag(uint8_t pos, uint8_t flags) const {
    return flags & 0b11 << pos;
  }

  void MqttFixed::setMsgType(MsgType type) {
    msgType = type;
  }

  MsgType MqttFixed::getMsgType() const {
    return msgType;
  }

  uint32_t MqttFixed::getVarInt(const VarInt &ref) const {
    return ref.get();
  }

  void MqttFixed::setVarInt(VarInt &ref, uint32_t val) {
    ref.set(val);
  }

  void MqttFixed::addVarInt(VarInt &ref, uint32_t octets, uint32_t prevOctets) {
    ref.add(octets, prevOctets);
  }

  uint32_t MqttFixed::getRemLength() const {
    return getVarInt(remLength);
  }
  
  void MqttFixed::addRemLength(uint32_t octets, uint32_t prevOctets) {
    addVarInt(remLength, octets, prevOctets);
  }
  
  void MqttFixed::setRemLength(uint32_t val) {
    setVarInt(remLength, val);
  }

  double MqttFixed::getCreationTime() const {
      return creationTime;
  }

  void MqttFixed::setCreationTime(double time) {
      creationTime = time;
  }


}
