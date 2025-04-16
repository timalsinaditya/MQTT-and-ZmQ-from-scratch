#ifndef VAR_INT
#define VAR_INT

#include <cstdint>
#include <stdlib.h>
#include <vector>

namespace mqtt
{
  class VarInt {
  private:
    std::vector<uint8_t> val;

  public:
    VarInt() {};
    ~VarInt() { val.clear(); };

    uint32_t get() const;
    uint8_t set(uint32_t d);
    uint8_t add(uint32_t a, uint32_t b = 0);
    uint8_t getSize() const;
  };
}

#endif // VAR_INT
