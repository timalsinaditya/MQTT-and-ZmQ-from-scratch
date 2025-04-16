#include "MqttWithProperties.h"
#include <omnetpp.h>

namespace mqtt
{

  void MqttWithProperties::WriteProperty(std::map<Property, std::any> &ref, Property type, std::any& val) {
    uint32_t size = 1;
    
    switch (type) {
    case PayloadFormatIndicator:
    case ReqProblemInfo:
    case ReqRespInfo:
    case MaxQoS:
    case RetainAvail:
    case WildcardSubAvail:
    case SubIdenAvail:
    case SharedSubAvail:
      {
	if(val.type() != typeid(uint8_t))
	  throw omnetpp::cRuntimeError("Invalid value for the property");
	size += 1;
      }
      
    case ServerKA:
    case RecvMax:
    case TopicAliasMax:
    case TopicAlias:
      {
	if(val.type() != typeid(uint16_t))
	  throw omnetpp::cRuntimeError("Invalid value for the property");
	size += 2;
      }
      
    case MsgExpInt:
    case SessionExpInt:
    case WillDelayInt:
    case MaxPackSize:
      {
	if(val.type() != typeid(uint32_t))
	  throw omnetpp::cRuntimeError("Invalid value for the property");
	size += 4;
      }
      
    case SubIden:
      {
	if(val.type() != typeid(VarInt))
	  throw omnetpp::cRuntimeError("Invalid value for the property");
	VarInt _val = std::any_cast<VarInt>(val);
	size += _val.getSize();
      }
      
    case ContentType:
    case ResponseTopic:
    case AssignedClientID:
    case AuthMethod:
    case RespInfo:
    case ServerRef:
    case ReasonStr:
      {
	if(val.type() != typeid(MqttString))
	  throw omnetpp::cRuntimeError("Invalid value for the property");
	size += 2;
	MqttString _val = std::any_cast<MqttString>(val);
	size += _val.get().size();
      }
      
    case UserProperty:
      {
	if(val.type() != typeid(std::pair<MqttString, MqttString>))
	  throw omnetpp::cRuntimeError("Invalid value for the property");
	size += 4;
	std::pair<MqttString, MqttString> _val = std::any_cast<std::pair<MqttString, MqttString>>(val);
	size += _val.first.get().size();
	size += _val.second.get().size();
      }
      
    case CorrelationData:
    case AuthData:
      {
	if(val.type() != typeid(MqttBin))
	  throw omnetpp::cRuntimeError("Invalid value for the property");
	size += 2;
	MqttBin _val = std::any_cast<MqttBin>(val);
	size += _val.get().size();
      }      
    }

    addRemLength(size);
    MqttFixed::addVarInt(propertyLen, size);
    ref[type] = val;
  }
  
  std::any MqttWithProperties::GetProperty(const std::map<Property, std::any> &ref, Property type) const {
    auto it = ref.find(type);
    if (it == ref.end())
      return {};
    return it->second;
  }

}
