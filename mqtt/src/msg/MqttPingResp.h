#ifndef MQTT_PINGRESP
#define MQTT_PINGRESP

#include "MqttFixed.h"
#include "../types/MsgType.h"

namespace mqtt
{
  class MqttPingResp : public MqttFixed {
  public:
    MqttPingResp() {
      setMsgType(MsgType::PINGRESP);
    }
  };
}

#endif // MQTT_PINGRESP
