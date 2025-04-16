#ifndef MQTT_PROPERTIES
#define MQTT_PROPERTIES

#include <cstdint>
#include <map>
#include <string>
#include <any>

namespace mqtt
{
  enum Property : uint8_t {
    PayloadFormatIndicator = 1,
    MsgExpInt = 2,
    ContentType = 3,
    ResponseTopic = 8,
    CorrelationData = 9,
    SubIden = 11,
    SessionExpInt = 17,
    AssignedClientID = 18,
    ServerKA = 19,
    AuthMethod = 21,
    AuthData = 22,
    ReqProblemInfo = 23,
    WillDelayInt = 24,
    ReqRespInfo = 25,
    RespInfo = 26,
    ServerRef = 28,
    ReasonStr = 31,
    RecvMax = 33,
    TopicAliasMax = 34,
    TopicAlias = 35,
    MaxQoS = 36,
    RetainAvail = 37,
    UserProperty = 38,
    MaxPackSize = 39,
    WildcardSubAvail = 40,
    SubIdenAvail = 41,
    SharedSubAvail = 42
  };

  // placeholder when the property is not present
  struct EmptyProperty { };
}

#endif
