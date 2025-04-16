#ifndef MQTT_PINGREQ
#define MQTT_PINGREQ

#include "MqttFixed.h"
#include "../types/MsgType.h"

namespace mqtt
{
  class MqttPingReq : public MqttFixed {
  public:
    MqttPingReq() {
      setMsgType(MsgType::PINGREQ);
    }
  };
}

#endif // MQTT_PINGREQ
