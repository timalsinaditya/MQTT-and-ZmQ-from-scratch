/*
 * ZmqSocket.h
 *
 *  Created on: Jan 22, 2025
 *      Author: autives
 */

#ifndef ZMQSOCKET_H_
#define ZMQSOCKET_H_

#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include <vector>

#include <omnetpp.h>

enum ZmqSocketType {
    ZMQ_PUB,
    ZMQ_SUB
};

class ZmqSocket {
public:
    ZmqSocketType socketType;

    inet::UdpSocket localSocket;

    // for ZMQ_SUB sockets
    std::vector<std::string> subscribedTopics;

    ZmqSocket(ZmqSocketType socketType);
    ~ZmqSocket();

    // for ZMQ_PUB sockets
    bool bind(int port);

    // for ZMQ_SUB sockets
    bool subscribe(const std::string& topic);
    bool shouldProcessTopic(std::string topic);

    void setOutputGate(omnetpp::cGate* gate);
    void setCallback(inet::UdpSocket::ICallback *callback);
};


#endif /* ZMQSOCKET_H_ */
