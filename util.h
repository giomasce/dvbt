
#ifndef _UTIL_H
#define _UTIL_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint8_t *buf;
  size_t buf_len;
  uint8_t *ref;
  uint32_t total_offset;
  FILE *fin;
  uint32_t reserved_back;
  uint32_t reserved_front;
} SlidingWindow;

size_t fread_exact(void *ptr, size_t size, size_t nmemb, FILE *stream);
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

#endif
