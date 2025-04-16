#include "MqttApp.h"
#include <omnetpp.h>
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/networklayer/common/L3AddressResolver.h"

using namespace omnetpp;

namespace mqtt
{
  void MqttApp::initialize(int stage)  {
    ClockUserModuleMixin::initialize(stage);

    if(stage == inet::INITSTAGE_LOCAL) {
      maxRetransmissions = par("maxRetransmissions");
      retransmissionInterval = par("retransmissionInterval");

      packetBER = par("packetBER");
    }

    init(stage);
  }

  void MqttApp::finish() {
    inet::ApplicationBase::finish();
  }

  void MqttApp::refreshDisplay() const {
    inet::ApplicationBase::refreshDisplay();
  }

  void MqttApp::socketClosed(inet::TcpSocket *socket) {
    if(operationalState == State::STOPPING_OPERATION) {
      startActiveOperationExtraTimeOrFinish(-1);
    }
  }

  void MqttApp::configureSocket() {
    localSocket.setOutputGate(gate("socketOut"));
    localSocket.setCallback(this);

    localSocket.bind(inet::L3Address(), par("localPort"));
  }

  void MqttApp::checkPacketIntegrity(const inet::B& receivedLen, const inet::B& fieldLen) {
    uint16_t rl = receivedLen.get();
    uint16_t fl = fieldLen.get();
    if(rl != fl + 2) { // 2 bytes for the fixed header
      EV << "Received length" << rl << "is not equal to the length of the packet" << fl + 2;
    }
  }

  void MqttApp::corruptPacket(inet::Packet *pk, double ber) {
    pk->setBitError(hasProbabilisticError(pk->getDataLength(), ber));
  }

  bool MqttApp::hasProbabilisticError(inet::b length, double ber) {
    double p = 1 - std::pow(1 - ber, length.get());
    double r = uniform(0, 1);
    if (r < p) erroneousPackets++;
    return r < p;
  }

  bool MqttApp::isSelfBroadcastAddress(const inet::L3Address& address) {
    return address == inet::L3Address("127.0.0.1");
  }

  uint16_t MqttApp::getNextPacketId(const std::set<uint16_t> &used, uint16_t &current, const std::string& error) {
    uint16_t max = UINT16_MAX - 1;
    if(used.size() >= max) {
      throw omnetpp::cRuntimeError("%s", error.c_str());
    }

    if(used.size() == 0) {
      current = (current >= max) ? 1 : current + 1;
      return current ;
    }

    if (current == 0 || current == max) {
      current = 1;
    }

    while(used.find(current) != used.end()) {
      current = (current >= max) ? 1 : current + 1;
    }

    return current;
  }

  // TODO
  std::vector<std::string> MqttApp::getPredefinedTopics() {
    return {};
  }
}
