
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

size_t fread_exact(void *ptr, size_t size, size_t nmemb, FILE *stream) {

  size_t read = 0;
  while (read < nmemb) {
    size_t new_read = fread(ptr, size, nmemb - read, stream);
    if (new_read == 0) {
      return read;
    }
    read += new_read;
    ptr = ((uint8_t*) ptr) + new_read * size;
  }

  return read;

}

SlidingWindow *sw_new(FILE *fin) {

  SlidingWindow *sw = malloc(sizeof(SlidingWindow));
  sw->fin = fin;
  sw->buf_len = 10;
  sw->buf = malloc(sw->buf_len);
  sw->ref = sw->buf;
  sw->total_offset = 0;
  sw->reserved_back = 0;
  sw->reserved_front = 0;

  return sw;

}

void sw_destroy(SlidingWindow *sw) {

  free(sw->buf);
  free(sw);

}

uint32_t sw_reserve_front(SlidingWindow *sw, size_t n) {

  if (n <= sw->reserved_front) {
    // We're requesting less than we already have reserved
  } else if (sw->ref + n <= sw->buf + sw->buf_len) {
    // We have enough memory to store the new data without compacting
    // or enlarging
    size_t read = fread_exact(sw->ref + sw->reserved_front, 1, n - sw->reserved_front, sw->fin);
    // In case of error, advance reserved_front only of the number of
    // actually read bytes
    sw->reserved_front = sw->reserved_front + read;
    if (read < n - sw->reserved_front) {
      return false;
    }
  } else {
    // First we try to compact
    memmove(sw->buf, sw->ref - sw->reserved_back, sw->reserved_back + sw->reserved_front);
    sw->ref = sw->buf + sw->reserved_back;
    // Do we also need to enlarge?
    if (sw->ref + n > sw->buf + sw->buf_len) {
      sw->buf_len = sw->reserved_back + n;
      sw->buf = realloc(sw->buf, sw->buf_len);
      if (sw->buf == NULL) {
        return false;
      }
      sw->ref = sw->buf + sw->reserved_back;
    }
    size_t read = fread_exact(sw->ref + sw->reserved_front, 1, n - sw->reserved_front, sw->fin);
    sw->reserved_front = sw->reserved_front + read;
    if (read < n - sw->reserved_front) {
      return false;
    }
  }

  return true;

}

uint32_t sw_reserve_back(SlidingWindow *sw, size_t n) {

  if ((sw->ref - sw->buf) >= n) {
    sw->reserved_back = n;
    return true;
  } else {
    return false;
  }

}

uint32_t sw_advance(SlidingWindow *sw, int32_t n) {

  if (n > 0) {
    if (!sw_reserve_front(sw, n)) {
      return false;
    }
  } else if (n < 0) {
    if (!sw_reserve_back(sw, -n)) {
      return false;
    }
  }

  sw->ref += n;
  sw->total_offset += n;
  sw->reserved_back += n;
  sw->reserved_front -= n;

  return true;

}

Heap *heap_new(size_t capacity) {

  Heap *heap = malloc(sizeof(Heap));
  heap->length = 0;
  heap->elements = malloc(sizeof(HeapElement) * capacity);
  return heap;

}

void heap_destroy(Heap *heap) {

  free(heap->elements);
  free(heap);

}

// TODO - Can we do it in-place?
void heap_swap(HeapElement *a, HeapElement *b) {

  HeapElement tmp;
  tmp.weight = a->weight;
  tmp.ptr = a->ptr;
  a->weight = b->weight;
  a->ptr = b->ptr;
  b->weight = tmp.weight;
  b->ptr = tmp.ptr;

}

void heap_up(Heap *heap, size_t i) {

  while (i > 0) {
    size_t parent = (i - 1) / 2;
    if (heap->elements[parent].weight > heap->elements[i].weight) {
      heap_swap(&heap->elements[parent], &heap->elements[i]);
      i = parent;
    } else {
      break;
    }
  }

}

void heap_down(Heap *heap, size_t i) {

  while (2*i+1 < heap->length) {
    size_t child = 2*i + 1;
    if ((2*i+2 < heap->length) && (heap->elements[2*i+2].weight < heap->elements[child].weight)) {
      child = 2*i + 2;
    }
    if (heap->elements[child].weight < heap->elements[i].weight) {
      heap_swap(&heap->elements[child], &heap->elements[i]);
      i = child;
    } else {
      break;
    }
  }

}

void heap_push(Heap *heap, double weight, void *ptr) {

  heap->elements[heap->length].weight = weight;
  heap->elements[heap->length].ptr = ptr;
  heap->length++;
  heap_up(heap, heap->length-1);

}

void heap_top(Heap *heap, double *weight, void **ptr) {

  *weight = heap->elements[0].weight;
  *ptr = heap->elements[0].ptr;

}

void heap_pop(Heap *heap) {

  heap->length--;
  if (heap->length > 0) {
    heap_swap(&heap->elements[0], &heap->elements[heap->length]);
    heap_down(heap, 0);
  }

}

double csqabs(double complex x) {

  double r = creal(x);
  double i = cimag(x);

  return r*r + i*i;

}
