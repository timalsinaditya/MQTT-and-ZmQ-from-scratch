#include "MqttSubscribe.h"

namespace mqtt
{
  void MqttSubscribe::setPacketIdentifier(uint16_t id) {
    packetIdentifier = id;
  }

  uint16_t MqttSubscribe::getPacketIdentifier() const {
    return packetIdentifier;
  }

  void MqttSubscribe::addTopicFilter(std::string &filter, uint8_t options) {
    topicFilters.push_back(std::make_pair(MqttString(filter), options));
    MqttFixed::addRemLength(1 + 2 + filter.length());
  }

  std::vector<std::pair<MqttString, uint8_t>> MqttSubscribe::getTopicFilters() const {
    return topicFilters;
  }
}
