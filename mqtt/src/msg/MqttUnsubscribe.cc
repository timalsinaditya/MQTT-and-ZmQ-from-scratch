#include "MqttUnsubscribe.h"

namespace mqtt
{
  void MqttUnsubscribe::setPacketIdentifier(uint16_t id) {
    packetIdentifier = id;
  }

  uint16_t MqttUnsubscribe::getPacketIdentifier() const {
    return packetIdentifier;
  }

  void MqttUnsubscribe::addTopicFilter(std::string &filter) {
    topicFilters.push_back(MqttString(filter));
    MqttFixed::addRemLength(filter.length());
  }
  
  const std::vector<MqttString>& MqttUnsubscribe::getTopicFilters() const {
    return topicFilters;
  }

}
