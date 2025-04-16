#ifndef SESSION_INFO_H
#define SESSION_INFO_H

#include "MsgInfo.h"

namespace mqtt
{
  struct SessionInfo {
    inet::TcpSocket *socket;
    std::string clientId;
    uint16_t keepAlive;

    std::map<uint16_t, RetransmissionInfo> retransmissions;
    std::map<uint16_t, MsgInfo> receivedQos2Messages;

    uint16_t lastPacketId = 0;
  };
}

#endif // SESSION_INFO_H
