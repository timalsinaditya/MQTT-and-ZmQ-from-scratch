#include "MqttString.h"
#include <omnetpp.h>

namespace mqtt
{  
  MqttString::MqttString(const std::string& val) {
    if(val.length() > UINT16_MAX)
       throw omnetpp::cRuntimeError("String length greater than maximum not allowed");
    str = val;
    len = val.length();
  }

  uint16_t MqttString::set(const std::string& val) {
    uint32_t newLen = val.length();
    uint16_t prevLen = str.length();
    
    if(newLen > UINT16_MAX)
      throw omnetpp::cRuntimeError("String length greater than maximum not allowed");
    len = newLen;
    str = val;
    return prevLen;
  }

  std::string MqttString::get() const {
    return str;
  }
}
