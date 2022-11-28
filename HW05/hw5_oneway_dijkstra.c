#include "hw5_oneway_dijkstra.h"

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "hw5_common.h"

static inline int compare_ushort_directly(unsigned short a, unsigned short b) {
    return (a > b) - (a < b);
}

static inline int compare_float_directly(float a, float b) { return (a > b) - (a < b); }

// Helper getters and setter for the DijkstraSearch structure

static inline float dijkstra_get_cost(DijkstraSearch* const s, SearchNode const n) {
    return s->costs[n.point_index][n.nodes_visited];
}

static inline void dijkstra_set_cost(DijkstraSearch* s, SearchNode const n, float cost) {
    s->costs[n.point_index][n.nodes_visited] = cost;
}

static inline unsigned dijkstra_get_idx(DijkstraSearch const* s, SearchNode const n) {
    return s->queue_indices[n.point_index][n.nodes_visited];
}

static inline void dijkstra_set_idx(DijkstraSearch* s, SearchNode const n, unsigned idx) {
    s->queue_indices[n.point_index][n.nodes_visited] = idx;
}

static inline void dijkstra_set_prev(DijkstraSearch* s, SearchNode const start, SearchNode const end) {
    s->previous[end.point_index][end.nodes_visited] = start;
}

// Methods for the queue

int dijkstra_compare_entries(DijkstraSearch* s, SearchNode const* a, SearchNode const* b) {
    // To ensure we get the best candidate in main Dijkstra loop, we need
    // the comparison to prefer the following properties, in order:
    // 1. More nodes can be visited
    // 2. More nodes have already been visited
    // 3. Less cost
    //
    // Since we'll be implementing a min-heap; this function should
    // return -1 if a is better than b; 1 if b is better than a or 0 if there are the same.

    // NOTE: This assumes points are sorted
    unsigned short a_possible_nodes = s->total_points - a->point_index - 1;
    unsigned short b_possible_nodes = s->total_points - b->point_index - 1;

    if (a_possible_nodes == b_possible_nodes && a->nodes_visited == b->nodes_visited) {
        // Case 3
        return compare_float_directly(dijkstra_get_cost(s, *a), dijkstra_get_cost(s, *b));
    } else if (a_possible_nodes == b_possible_nodes) {
        // Case 2
        return compare_ushort_directly(b->nodes_visited, a->nodes_visited);
    } else {
        // Case 1
        return compare_ushort_directly(b_possible_nodes, a_possible_nodes);
    }
}

void dijkstra_after_swap(DijkstraSearch* s, size_t i, size_t j) {
    dijkstra_set_idx(s, s->queue_data[i], i);
    dijkstra_set_idx(s, s->queue_data[j], j);
}

// Helpers for the search

static inline void dijkstra_push_start(DijkstraSearch* s, unsigned short start) {
    SearchNode n = {.point_index = start, .nodes_visited = 1};
    dijkstra_set_cost(s, n, 0.0);
    s->previous[start][1] = (SearchNode){.nodes_visited = 0};
    heap_push(&s->queue, &n);
}

static inline SearchNode dijkstra_pop_min(DijkstraSearch* s) {
    SearchNode popped;
    heap_pop(&s->queue, &popped, 0);
    dijkstra_set_idx(s, popped, DIJKSTRA_INDEX_INVALID);
    return popped;
}

static inline DijkstraSearchResult dijkstra_generate_result(DijkstraSearch* s, SearchNode end) {
    DijkstraSearchResult result;
    result.cost = dijkstra_get_cost(s, end);
    dijkstra_reconstruct_path(s, &result, end);
    return result;
}

static bool dijkstra_permissable_and_better(DijkstraSearch* s, SearchNode next, float next_cost) {
    return (next_cost <= s->max_cost)  // Permissable cost
           && (s->max_length == DIJKSTRA_NO_MAX_LEN ||
               next.nodes_visited <= s->max_length)      // Permissable length
           && (next_cost < dijkstra_get_cost(s, next));  // Is better
}

static inline void dijkstra_push(DijkstraSearch* s, SearchNode next, float next_cost) {
    unsigned existing_idx = dijkstra_get_idx(s, next);
    dijkstra_set_cost(s, next, next_cost);

    if (existing_idx == DIJKSTRA_INDEX_INVALID) {
        dijkstra_set_idx(s, next, s->queue.length);
        heap_push(&s->queue, &next);
    } else {
        assert(existing_idx < s->queue.length);
        heap_sift_up(&s->queue, existing_idx);
    }
}

// External interface

void dijkstra_init(DijkstraSearch* s, unsigned short total_points,
                   dijkstra_edge_getter edge_getter, void* edge_getter_ctx) {
    s->queue = (Heap){
        .data = s->queue_data,
        .capacity = DIJKSTRA_QUEUE_CAPACITY,
        .element_size = sizeof(SearchNode),
        .compare_elements = (heap_comparator)dijkstra_compare_entries,
        .after_swap = (heap_swap_handler)dijkstra_after_swap,
        .context = s,
    };
    s->total_points = total_points;
    s->get_edge = edge_getter;
    s->get_edge_context = edge_getter_ctx;
}

static void dijkstra_reconstruct_path(DijkstraSearch* s, DijkstraSearchResult* into, SearchNode end) {
    into->route_length = end.nodes_visited;
    do {
        into->route[end.nodes_visited - 1] = end.point_index;
        end = s->previous[end.point_index][end.nodes_visited];
    } while (end.nodes_visited);
}

void dijkstra_before_search(DijkstraSearch* s, float max_cost, unsigned short max_length) {
    s->max_cost = max_cost;
    s->max_length = max_length;

    // Initialize the costs and queue indices
    for (unsigned short i = 0; i < MAX_POINTS_LEN; ++i) {
        for (unsigned short j = 0; j < MAX_POINTS_LEN; ++j) {
            s->costs[i][j] = INFINITY;
            s->queue_indices[i][j] = UINT_MAX;
        }
    }

    s->queue.length = 0;
}

DijkstraSearchResult dijkstra_search(DijkstraSearch* s, unsigned short start, unsigned short end) {
    // Create the initial entry
    dijkstra_push_start(s, start);

    // Loop while there are element
    while (s->queue.length > 0) {
        // Get the best vertex
        SearchNode popped = dijkstra_pop_min(s);

        // Check if the goal was reached
        if (popped.point_index == end) return dijkstra_generate_result(s, popped);

        // Iterate over all possible neighbors
        for (unsigned short next_idx = 0; next_idx < s->total_points; ++next_idx) {
            SearchNode next = {.point_index = next_idx, .nodes_visited = popped.nodes_visited + 1};
            float edge_cost = s->get_edge(s->get_edge_context, popped, next);

            // See if edge even exists
            if (isinf(edge_cost)) continue;

            // Calculate the total cost of the edge
            float next_cost = dijkstra_get_cost(s, popped) + edge_cost;

            // Insert into queue if `new_entry` is better
            if (dijkstra_permissable_and_better(s, next, next_cost)) {
                dijkstra_push(s, next, next_cost);
                dijkstra_set_prev(s, popped, next);
            }
        }
    }

    // No path
    return (DijkstraSearchResult){.cost = INFINITY, .route_length = 0};
}
