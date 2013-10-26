
#ifndef _UTIL_H
#define _UTIL_H

#include <stdbool.h>
#include <stdint.h>
#include <complex.h>

// Macros for bitvectors
#define bv_get(bv, pos) ((bv)[(pos)/8] & (1 << ((pos)%8)))
#define bv_set(bv, pos, val) (bv)[(pos)/8] = (((bv)[(pos)/8] | ((!!(val)) << ((pos)%8))) & (((!(val)) << ((pos)%8)) ^ 0xff))

typedef struct {
  uint8_t *buf;
  size_t buf_len;
  uint8_t *ref;
  uint32_t total_offset;
  FILE *fin;
  uint32_t reserved_back;
  uint32_t reserved_front;
} SlidingWindow;

SlidingWindow *sw_new(FILE *fin);
void sw_destroy(SlidingWindow *sw);
uint32_t sw_reserve_front(SlidingWindow *sw, size_t n);
uint32_t sw_reserve_back(SlidingWindow *sw, size_t n);
uint32_t sw_advance(SlidingWindow *sw, int32_t n);

typedef struct {
  double weight;
  void *ptr;
} HeapElement;

typedef struct {
  size_t length;
  HeapElement *elements;
} Heap;

Heap *heap_new(size_t capacity);
void heap_destroy(Heap *heap);
void heap_push(Heap *heap, double weight, void *ptr);
void heap_top(Heap *heap, double *weight, void **ptr);
void heap_pop(Heap *heap);

double csqabs(complex x);

#endif
