/*
 * MsgType.h
 *
 *  Created on: Jul 9, 2024
 *      Author: autives
 */

#ifndef MSGTYPE_H_
#define MSGTYPE_H_

enum MsgType : uint8_t {
    RESSERVED,
    CONNECT,
    CONNACK,
    PUBLISH,
    PUBACK,
    PUBREC,
    PUBREL,
    PUBCOMP,
    SUBSCRIBE,
    SUBACK,
    UNSUBSCRIBE,
    UNSUBACK,
    PINGREQ,
    PINGRESP,
    DISCONNECT,
    AUTH
};

#endif /* MSGTYPE_H_ */
