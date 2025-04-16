/*
 * MqttFixed.h
 *
 *  Created on: Jul 9, 2024
 *      Author: autives
 */

#ifndef MSG_MQTTFIXED_H_
#define MSG_MQTTFIXED_H_

#include "MqttProperties.h"
#include "inet/common/packet/chunk/Chunk_m.h"
#include "../types/MsgType.h"
#include "../types/Flag.h"
#include "VarInt.h"
#include <any>
#include <map>

namespace mqtt {

  class MqttFixed : public inet::FieldsChunk
  {
  private:
    MsgType msgType;
    VarInt remLength;

    double creationTime;

  public:
    uint8_t flags;

  public:
    void setFlag(uint8_t value, uint8_t pos, uint8_t &flags);
    uint8_t getFlag(uint8_t pos, uint8_t flags) const;

    void setMsgType(MsgType type);
    MsgType getMsgType() const;

    uint32_t getRemLength() const;
    void addRemLength(uint32_t octets, uint32_t prevOctets = 0);
    void setRemLength(uint32_t val);

    uint32_t getVarInt(const VarInt &ref) const;
    void addVarInt(VarInt &ref, uint32_t octets, uint32_t prevOctets = 0);
    void setVarInt(VarInt &ref, uint32_t val);

    double getCreationTime() const;
    void setCreationTime(double time);

    MqttFixed() { setVarInt(remLength, 0); }
    virtual ~MqttFixed() {}
  };

}

#endif /* MSG_MQTTFIXED_H_ */
