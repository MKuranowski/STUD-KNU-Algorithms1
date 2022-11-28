#include "hw5_common.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>

noreturn void exit_with_message(char const* msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

// Point-related helpers

int compare_points(void const* a_ptr, void const* b_ptr) {
    Point const* a = a_ptr;
    Point const* b = b_ptr;

    if (a->x == b->x) {
        return (a->y > b->y) - (a->y < b->y);
    } else {
        return (a->x > b->x) - (a->x < b->x);
    }
}

extern inline float distance_between(Point a, Point b);

void load_points(FILE* f, Point points[MAX_POINTS_LEN], unsigned* length) {
    // Load points count and verify it
    unsigned points_in_file;
    if (fscanf(f, "%u", &points_in_file) != 1)
        exit_with_message("Failed to load point count from input file");
    if (points_in_file > MAX_POINTS_LEN) exit_with_message("Too many points in input file");

    // Load the points
    for (size_t i = 0; i < points_in_file; ++i) {
        if (fscanf(f, "%f %f", &points[i].x, &points[i].y) != 2)
            exit_with_message("Failed to load point from file");
    }

    // Sort points (needed for fast evaluation of whether to add a node to a queue)
    qsort(points, points_in_file, sizeof(Point), compare_points);

    // Save the number of loaded points
    *length = points_in_file;
}

// Heap-related helpers

void memswap(void* restrict a, void* restrict b, size_t bytes) {
    char tmp[bytes];
    memcpy(tmp, a, bytes);
    memcpy(a, b, bytes);
    memcpy(b, tmp, bytes);
}

static inline void heap_swap(Heap* h, size_t i, size_t j) {
    assert(i != j);
    assert(i < h->length);
    assert(j < h->length);

    memswap(h->data + i * h->element_size, h->data + j * h->element_size, h->element_size);
    if (h->after_swap) h->after_swap(h->context, i, j);
}

static inline int heap_cmp(Heap* h, size_t i, size_t j) {
    return h->compare_elements(h->context, h->data + i * h->element_size, h->data + j * h->element_size);
}

void heap_sift_down(Heap* h, size_t index) {
    bool did_swap;

    do {
        assert(index < h->length);
        size_t left = 2 * index + 1;
        size_t right = left + 1;

        size_t smallest = index;

        // Find the smallest element
        if (left < h->length && heap_cmp(h, left, smallest) < 0) smallest = left;
        if (right < h->length && heap_cmp(h, right, smallest) < 0) smallest = right;

        // If smallest isn't root - swap it and keep recursing down
        if (smallest != index) {
            heap_swap(h, index, smallest);
            index = smallest;
            did_swap = true;
        } else {
            did_swap = false;
        }
    } while (did_swap);
}

void heap_sift_up(Heap* h, size_t index) {
    assert(index < h->length);
    size_t parent_idx = (index - 1) / 2;

    // Swap `heap[index]` with its parent while it's smaller
    while (index > 0 && heap_cmp(h, index, parent_idx) < 0) {
        heap_swap(h, index, parent_idx);

        index = parent_idx;
        parent_idx = (index - 1) / 2;
    }
}

void heap_push(Heap* restrict h, void* restrict element) {
    assert(h->length < h->capacity);

    memcpy(h->data + h->length * h->element_size, element, h->element_size);
    ++h->length;

    heap_sift_up(h, h->length - 1);
}

void heap_pop(Heap* restrict h, void* restrict target, size_t index) {
    assert(index < h->length);

    memcpy(target, h->data + index * h->element_size, h->element_size);

    // No need to restore heap invariant if last element was removed
    if (h->length - 1 != index) {
        heap_swap(h, index, h->length - 1);
        --h->length;
        heap_sift_down(h, index);
    } else {
        --h->length;
    }
}
