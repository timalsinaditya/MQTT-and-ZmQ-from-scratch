/*
 * ZmqSocket.cc
 *
 *  Created on: Jan 22, 2025
 *      Author: autives
 */

#include "ZmqSocket.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include <omnetpp.h>
#include <algorithm>

ZmqSocket::ZmqSocket(ZmqSocketType type) : socketType(type) {
    if (type == ZMQ_PUB) {
        localSocket.setOutputGate(nullptr);
    }
}

ZmqSocket::~ZmqSocket() {}

bool ZmqSocket::bind(int port) {
    if (socketType != ZMQ_PUB) return false;

    try {
        localSocket.bind(port);
        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}

bool ZmqSocket::subscribe(const std::string& topic) {
    if (socketType != ZMQ_SUB) return false;

    subscribedTopics.push_back(topic);
    return true;
}

bool ZmqSocket::shouldProcessTopic(std::string topic) {
    return std::find(subscribedTopics.begin(), subscribedTopics.end(), topic) != subscribedTopics.end();
}

void ZmqSocket::setOutputGate(omnetpp::cGate* gate) {
    localSocket.setOutputGate(gate);
}

void ZmqSocket::setCallback(inet::UdpSocket::ICallback *callback) {
    if (socketType == ZMQ_PUB) {
        localSocket.setCallback(callback);
    }

    // for SUB sockets, it should already be set before calling addSocket
}



