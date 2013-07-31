
#include "prbs.h"

// From http://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
inline uint32_t count_ones(uint32_t i) {

  i = i - ((i >> 1) & 0x55555555);
  i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
  return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;

}

inline uint32_t prbs_gen(uint32_t *state, uint32_t taps, uint8_t length) {

  uint32_t next = count_ones(*state & taps) % 2;
  *state <<= 1;
  uint32_t res = *state & (0x1 << length);
  *state |= next;
  return res;

}
