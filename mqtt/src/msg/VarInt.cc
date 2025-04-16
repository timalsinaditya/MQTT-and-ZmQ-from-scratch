#include "VarInt.h"
#include <cstdint>
#include <omnetpp.h>

namespace mqtt
{
  uint8_t VarInt::set(uint32_t d) {
    if(val.size() > 0)
      val.clear();

    do {
      uint32_t value = d % 128;
      d /= 128;
      if(d > 0)
	value |= 128;
      val.push_back(value);
    } while (d > 0);

    return val.size();
  }

  uint32_t VarInt::get() const {
    uint32_t multiplier = 1, value = 0, i = 0;
    do {
      value += (val[i] & 127) * multiplier;
      if(multiplier > 128*128*128)
	throw omnetpp::cRuntimeError("Malformed Variable Byte Integer");
      multiplier *= 128;
    } while((val[i++] & 128) != 0);
    
    return value;
  }

  uint8_t VarInt::add(uint32_t a, uint32_t b) {
    if (a == b) return val.size();
    uint32_t current = get();

    if(current < b)
      throw omnetpp::cRuntimeError("Previous octets cannot exceed current");

    return set(current - b + a);
  }

  uint8_t VarInt::getSize() const {
    return val.size();
  }
}
