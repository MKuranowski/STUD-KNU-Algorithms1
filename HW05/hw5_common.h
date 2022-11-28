#pragma once

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdnoreturn.h>

/**
 * exit_with_message prints the provided message to stderr followed by a newline,
 * then terminates the program by calling exit
 */
noreturn void exit_with_message(char const* msg);

// Point-related features

/**
 * The expected number of points
 */
#define MAX_POINTS_LEN 100

static_assert(MAX_POINTS_LEN < USHRT_MAX, "unsigned short is used to index points");

/**
 * Point represents a tuple of 2 floats
 */
typedef struct {
    float x;
    float y;
} Point;

/**
 * Compares 2 points lexicographically.
 */
int compare_points(void const* a_ptr, void const* b_ptr);

/**
 * Calculates the Euclidean distance between 2 points.
 */
inline float distance_between(Point a, Point b) { return hypotf(a.x - b.x, a.y - b.y); }

/**
 * Loads points from a given file.
 */
void load_points(FILE* f, Point points[MAX_POINTS_LEN], unsigned* length);

// Generic min-heap implementation

/**
 * heap_comparator should compare elements in the heap.
 * First argument will be passed through from `heap.context`.
 *
 * Must return:
 * - negative number when a < b
 * - zero when a == b
 * - positive number when a > b
 */
typedef int (*heap_comparator)(void* context, void const* a, void const* b);

/**
 * heap_swap_handler is called after 2 elements from the heap were swapped.
 * It may be set to null. The first argument is passed through from `heap.context`
 */
typedef void (*heap_swap_handler)(void*, size_t, size_t);

/**
 * Heap is a data structure, such that `heap.data[0]` is always the smallest element.
 *
 * The implementation guarantees never to exceed the provided capacity.
 */
typedef struct {
    void* data;
    size_t capacity;
    size_t element_size;

    size_t length;

    heap_comparator compare_elements;
    heap_swap_handler after_swap;
    void* context;
} Heap;

/**
 * Swaps the contents of one memory location with another memory location.
 */
void memswap(void* restrict from, void* restrict to, size_t bytes);

/**
 * Pushes an element into the MinHeap
 */
void heap_push(Heap* restrict h, void* restrict element);

/**
 * Pops `heap[index]` element from the heap into `target`.
 * Use index == 0 to pop the smallest element.
 */
void heap_pop(Heap* restrict h, void* restrict target, size_t index);

/**
 * Moves an element at `index` down until heap invariant is maintained.
 * should be called after heap[index] got its value increased.
 */
void heap_sift_down(Heap* h, size_t index);

/**
 * Moves an element at `index` up until heap invariant is maintained.
 * should be called after heap[index] got its value decreased.
 */
void heap_sift_up(Heap* h, size_t index);
