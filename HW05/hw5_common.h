/*
 * ID: 2020427681
 * NAME: Mikolaj Kuranowski
 * OS: Debian 11
 * Compiler version: gcc 12.2.0
 */
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

inline int compare_int_directly(int a, int b) { return (a > b) - (a < b); }
inline int compare_float_directly(float a, float b) { return (a > b) - (a < b); }

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
typedef int (*heap_comparator)(void const* a, void const* b);

/**
 * heap_on_index_update is called after an element's position in the queue has changed.
 * If an element was removed, new_index will be set to `(size_t)-1`.
 */
typedef void (*heap_on_index_update)(void* element, size_t new_index);

/**
 * Heap is a container for pointers, such that `heap.data[0]` is always the smallest element.
 *
 * The implementation guarantees never to exceed the provided capacity.
 *
 * compare_elements must not be NULL, while on_index_update may be NULL.
 */
typedef struct {
    void** data;
    size_t capacity;

    size_t length;

    heap_comparator compare_elements;
    heap_on_index_update on_index_update;
} Heap;

/**
 * Pushes an element into the MinHeap
 */
void heap_push(Heap* h, void* element);

/**
 * Pops `heap[index]` element from the heap into `target`.
 * Use index == 0 to pop the smallest element.
 */
void* heap_pop(Heap* h, size_t index);

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
