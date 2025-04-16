/*
 * ZmqVehicleApp.cc
 *
 *  Created on: Jan 23, 2025
 *      Author: autives
 */


#include "ZmqVehicleApp.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "./msg/ZmqMessage.h"

#include <stdio.h>

namespace zmq {

Define_Module(ZmqVehicleApp);

ZmqVehicleApp::ZmqVehicleApp() {

}

ZmqVehicleApp::~ZmqVehicleApp() {}

void ZmqVehicleApp::initialize(int stage) {
    ClockUserModuleMixin<ApplicationBase>::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        publishDataSize = par("publishDataSize");
        packetsReceived = 0;
        packetsSent = 0;
    }
    else if (stage == inet::INITSTAGE_APPLICATION_LAYER) {
        // Schedule periodic vehicle data publishing
        publishEvent = new inet::ClockEvent("publishEvent");
        publishInterval = par("publishInterval");

        endToEndDelay.setName("End to End Delay");

        destAddress = inet::L3AddressResolver().resolve(par("destAddress"));
        localPort = par("localPort").intValue();
        destPort = par("destPort").intValue();

        publisherSocket.setOutputGate(gate("socketOut"));
        subscriberSocket.setOutputGate(gate("socketOut"));

        publisherSocket.bind(localPort);
        subscriberSocket.bind(destPort);

        publisherSocket.setCallback(this);
        subscriberSocket.setCallback(this);

        inet::IInterfaceTable *ift = inet::getModuleFromPar<inet::IInterfaceTable>(par("interfaceTableModule"), this);
        inet::MulticastGroupList mgl = ift->collectMulticastGroups();
        publisherSocket.joinLocalMulticastGroups(mgl);
        subscriberSocket.joinLocalMulticastGroups(mgl);

        // if the multicastInterface parameter is not empty, set the interface explicitly
        const char *multicastInterface = par("multicastInterface");
        if (multicastInterface[0]) {
            inet::NetworkInterface *ie = ift->findInterfaceByName(multicastInterface);
            if (!ie)
                throw omnetpp::cRuntimeError("Wrong multicastInterface setting: no interface named \"%s\"", multicastInterface);
            publisherSocket.setMulticastOutputInterface(ie->getInterfaceId());
            subscriberSocket.setMulticastOutputInterface(ie->getInterfaceId());
        }

        auto topics = getPredefinedTopics();
        for (const auto& topic : topics) {
            subscribedTopics.push_back(topic);
        }

        scheduleClockEventAfter(publishInterval, publishEvent);
    }
}

void ZmqVehicleApp::configureSocket() {
    publisherSocket.setCallback(this);
    subscriberSocket.setCallback(this);

    publisherSocket.setOutputGate(gate("socketOut"));
    subscriberSocket.setOutputGate(gate("socketOut"));

    int localPort = par("localPort").intValue();
    publisherSocket.bind(localPort);

    destPort = par("destPort").intValue();
    subscriberSocket.bind(destPort);
}


  void ZmqVehicleApp::socketDataArrived(inet::UdpSocket *socket, inet::Packet *packet) {
      ZmqMessage msg = parseZmqPacket(packet);
      handleZmqMessage(msg);
      packetsReceived++;
      delete packet;
  }

  void ZmqVehicleApp::socketErrorArrived(inet::UdpSocket *socket, inet::Indication *indication) {
      EV_WARN << "UDP socket error arrived\n";
      delete indication;
  }

  void ZmqVehicleApp::socketClosed(inet::UdpSocket *socket) {
      // Handle socket closure
      socket->close();
  }

std::vector<std::string> ZmqVehicleApp::getPredefinedTopics() {
    std::vector<std::string> topics;
    std::string topicsStr = par("subscribeTopics").stdstringValue();

    // Parse comma-separated topics
    size_t pos = 0;
    while ((pos = topicsStr.find(',')) != std::string::npos) {
        topics.push_back(topicsStr.substr(0, pos));
        topicsStr.erase(0, pos + 1);
    }
    if (!topicsStr.empty()) {
        topics.push_back(topicsStr);
    }

    return topics;
}

void ZmqVehicleApp::handleMessageWhenUp(omnetpp::cMessage *msg) {
    if (msg == publishEvent) {
        publishVehicleData();
        scheduleClockEventAfter(publishInterval, publishEvent);
    }
    else if (subscriberSocket.belongsToSocket(msg))
        subscriberSocket.processMessage(msg);
}

void ZmqVehicleApp::publishVehicleData() {
    ZmqMessage msg;

    // Add topic frame
    ZmqFrame topicFrame;
    topicFrame.more = true;
    topicFrame.data = "vehicle_data";
    msg.frames.push_back(topicFrame);

    // Add data frame
    ZmqFrame dataFrame;
    dataFrame.more = true;
    dataFrame.data = generatePublishData(); // Replace with actual vehicle data
    msg.frames.push_back(dataFrame);

    ZmqFrame timeFrame;
    timeFrame.more = false;
    timeFrame.data = std::to_string(omnetpp::simTime().dbl());
    msg.frames.push_back(timeFrame);

    inet::Packet *packet = createZmqPacket(msg);
    publisherSocket.sendTo(packet, destAddress, destPort);
    packetsSent++;
}

void ZmqVehicleApp::handleZmqMessage(const ZmqMessage& msg) {
    processZmqMessage(msg);
}

bool ZmqVehicleApp::shouldProcessTopic(std::string topic) {
    return std::find(subscribedTopics.begin(), subscribedTopics.end(), topic) != subscribedTopics.end();
}

std::string ZmqVehicleApp::padData(const std::string& data, uint32_t requiredSize) {
    std::string paddedData = data;
    while (paddedData.size() < requiredSize) {
        paddedData += data;
    }
    return paddedData.substr(0, requiredSize);
}

std::string ZmqVehicleApp::generatePublishData() {
      uint32_t dataSize = publishDataSize;
      std::ostringstream dataStream;
      std::srand(std::time(0));

      float temperature = uniform(-20.0, 50.0);
      float speed = uniform(0.0, 200.0);
      float locationLat = uniform(-90.0, 90.0);
      float locationLon = uniform(-180.0, 180.0);
      float fuelLevel = uniform(0.0, 100.0);
      int brakeStatus = intuniform(0, 1);

      dataStream << "{"
                 << "\"temperature\":" << temperature << ","
                 << "\"speed\":" << speed << ","
                 << "\"location\": {"
                 << "\"lat\":" << locationLat << ","
                 << "\"lon\":" << locationLon << "},"
                 << "\"fuelLevel\":" << fuelLevel << ","
                 << "\"brakeStatus\":" << brakeStatus
                 << "}";
      std::string data = dataStream.str();

      if (data.size() < dataSize) {
          return padData(data, dataSize);
      } else if (data.size() > dataSize) {
          return data.substr(0, dataSize);
      }

      return data;
  }

void ZmqVehicleApp::processZmqMessage(const ZmqMessage& msg) {
    if (!msg.frames.empty()) {
        std::string topic = msg.frames[0].data;
        std::string time = msg.frames[2].data;
        double simStartTime = std::stod(time);
        double delay = omnetpp::simTime().dbl() - simStartTime;
        endToEndDelay.record(delay);

        if (shouldProcessTopic(topic)) {
            EV_INFO << "Received for topic " << topic << " :" << std::endl;
            if (msg.frames.size() > 1) {
                std::string data = msg.frames[1].data;
                EV_INFO << data << std::endl;
             }
        }
    }
    packetsReceived++;
}

inet::Packet* ZmqVehicleApp::createZmqPacket(const ZmqMessage& msg) {
    auto payload = inet::makeShared<ZmqMessage>();
    payload->frames = msg.frames;
    payload->setChunkLength(payload->calculateChunkLength());

    inet::Packet *packet = new inet::Packet("ZMQMessagePacket");
    packet->insertAtBack(payload);
    return packet;
}

ZmqMessage ZmqVehicleApp::parseZmqPacket(inet::Packet* packet) {
    auto payload = packet->peekAtFront<ZmqMessage>();
    ZmqMessage msg;
    msg.frames = payload->frames;
    return msg;
}

void ZmqVehicleApp::refreshDisplay() const {
    inet::ApplicationBase::refreshDisplay();
}

void ZmqVehicleApp::finish() {
    recordScalar("packets received", packetsReceived);
    recordScalar("packets sent", packetsSent);
}

} // namespace zmq
