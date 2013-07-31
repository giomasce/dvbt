
#ifndef _PRBS_H
#define _PRBS_H

#include <stdint.h>

uint32_t prbs_gen(uint32_t *state, uint32_t taps, uint8_t length);

#endif
