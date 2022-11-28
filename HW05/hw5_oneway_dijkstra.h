#pragma once

#include <assert.h>
#include <limits.h>
#include <stddef.h>

#include "hw5_common.h"

#define DIJKSTRA_QUEUE_CAPACITY (MAX_POINTS_LEN * MAX_POINTS_LEN)

static_assert(MAX_POINTS_LEN < USHRT_MAX, "unsigned short will be used to index points");
static_assert(DIJKSTRA_QUEUE_CAPACITY < UINT_MAX,
              "unsigned int will be used to index queue entries");

typedef struct {
    unsigned short point_index;
    unsigned short nodes_visited;
} SearchNode;

/**
 * Function which should calculate the cost of an edge.
 * May return `INFINITY` if no such edge exists.
 */
typedef float (*dijkstra_edge_getter)(void* context, SearchNode from, SearchNode to);

#define DIJKSTRA_NO_MAX_LEN USHRT_MAX

#define DIJKSTRA_INDEX_INVALID UINT_MAX

typedef struct {
    SearchNode queue_data[DIJKSTRA_QUEUE_CAPACITY];
    SearchNode previous[MAX_POINTS_LEN][MAX_POINTS_LEN];
    float costs[MAX_POINTS_LEN][MAX_POINTS_LEN];
    unsigned queue_indices[MAX_POINTS_LEN][MAX_POINTS_LEN];
    Heap queue;
    unsigned short total_points;

    float max_cost;
    unsigned short max_length;

    dijkstra_edge_getter get_edge;
    void* get_edge_context;
} DijkstraSearch;

typedef struct {
    float cost;
    unsigned short route[MAX_POINTS_LEN];
    unsigned short route_length;
} DijkstraSearchResult;

/**
 * Initializes the DijkstraSearch data structure.
 * Must be called at least once before the first search.
 */
void dijkstra_init(DijkstraSearch* s, unsigned short total_points,
                   dijkstra_edge_getter edge_getter, void* edge_getter_ctx);

/**
 * Resets the DijkstraSearch state.
 * Must be called before every search.
 */
void dijkstra_before_search(DijkstraSearch* s, float max_cost, unsigned short max_length);

DijkstraSearchResult dijkstra_search(DijkstraSearch* s, unsigned short start, unsigned short end);

int dijkstra_compare_entries(DijkstraSearch*, SearchNode const*, SearchNode const*);

void dijkstra_after_swap(DijkstraSearch*, size_t, size_t);

static void dijkstra_reconstruct_path(DijkstraSearch* s, DijkstraSearchResult* into, SearchNode end);
