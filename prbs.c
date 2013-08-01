
#include "prbs.h"

// From http://graphics.stanford.edu/~seander/bithacks.html
uint8_t compute_parity(uint32_t v) {

  v ^= v >> 16;
  v ^= v >> 8;
  v ^= v >> 4;
  v &= 0xf;
  return (0x6996 >> v) & 1;

}

inline uint32_t prbs_gen(uint32_t *state, uint32_t taps, uint8_t length) {

  uint8_t next = compute_parity(*state & taps);
  *state <<= 1;
  uint32_t res = *state & (1 << length);
  *state |= next;
  return res;

}
