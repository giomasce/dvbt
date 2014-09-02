
#include <stdlib.h>

#include "debug.h"

void debug_dump_phase_diagrams(OFDMContext *ctx, SlidingWindow *sw, int steps_from, int steps_to) {

  int i;
  char filename[1024];

  sw_reserve_front(sw, 2 * ctx->full_len + steps_to);
  sw_reserve_back(sw, 2 * ctx->full_len + steps_from);
  for (i = steps_from; i < steps_to; i++) {
    int idx = i - steps_from;
    ofdm_context_decode_symbol(ctx, i);
    snprintf(filename, 1024, "dump/step_%06d", idx);
    ofdm_context_dump_freqs(ctx, filename);
    snprintf(filename, 1024, "gnuplot -e 'set terminal png size 1024,768 enhanced font \"Helvetica,20\" ; set output \"dump/step_%06d.png\" ; plot \"dump/step_%06d\" using 1:2'", idx, idx);
    system(filename);
  }

}
