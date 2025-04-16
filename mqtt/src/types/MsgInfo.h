#ifndef MSG_INFO_H
#define MSG_INFO_H

#include "MsgType.h"

namespace mqtt
{
  struct MsgInfo {
    MsgType type;
    uint16_t packetId = 0;
    uint8_t reasonCode = 0;

    std::string topic = "";
    std::string data = "";
    uint8_t qos;
    bool dup;
    bool retain;

    uint8_t subOpts;
  };

  struct RetransmissionInfo {
    inet::ClockEvent *retransmissionEvent = nullptr;
    uint8_t retransmissionCounter = 0;

    MsgInfo msg;
  };
   
}

#endif // MSG_INFO_H
