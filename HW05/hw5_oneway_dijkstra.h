#pragma once

#include <assert.h>
#include <limits.h>
#include <stddef.h>

#include "hw5_common.h"

#define DIJKSTRA_QUEUE_CAPACITY (MAX_POINTS_LEN * MAX_POINTS_LEN)

static_assert(MAX_POINTS_LEN < USHRT_MAX, "unsigned short will be used to index points");
static_assert(DIJKSTRA_QUEUE_CAPACITY < UINT_MAX,
              "unsigned int will be used to index queue entries");

/**
 * The actual node used in Dijkstra search - tuple of
 * the point and how many nodes were visited.
 */
typedef struct {
    unsigned short point_index;
    unsigned short nodes_visited;
} SearchNode;

/**
 * Function which should calculate the cost of an edge.
 * May return `INFINITY` if no such edge exists.
 */
typedef float (*dijkstra_edge_getter)(void* context, SearchNode from, SearchNode to);

/**
 * Special value to signal that routes of any lengths are permitted.
 */
#define DIJKSTRA_NO_MAX_LEN USHRT_MAX

/**
 * Special value to signal that a given index entry was not found.
 */
#define DIJKSTRA_INDEX_INVALID UINT_MAX

/**
 * A structure with all the necessary details to perform a Dijkstra search.
 */
typedef struct {
    /// Space reserved for the priority queue.
    SearchNode queue_data[DIJKSTRA_QUEUE_CAPACITY];

    /// Mapping from one SearchNode to its predecessor.
    SearchNode previous[MAX_POINTS_LEN][MAX_POINTS_LEN];

    /// Costs of getting to a specific node.
    float costs[MAX_POINTS_LEN][MAX_POINTS_LEN];

    /// Index of a SearchNode into queue_data.
    /// May be DIJKSTRA_INDEX_INVALID.
    unsigned queue_indices[MAX_POINTS_LEN][MAX_POINTS_LEN];

    /// Priority queue used in the mail Dijkstra loop.
    Heap queue;

    /// Total number of points, used for comparing SearchNodes.
    unsigned short total_points;

    /// Maximum permitted cost of a solution.
    /// May be HUGE_VALF or INFINITY.
    float max_cost;

    /// Maximum permitted length of a solution.
    /// May be DIJKSTRA_NO_MAX_LEN.
    unsigned short max_length;

    /// A getter function to get an edge cost.
    dijkstra_edge_getter get_edge;

    /// Argument passed through to get_edge.
    void* get_edge_context;
} DijkstraSearch;

/**
 * Return value of dijkstra_search.
 */
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
 * Resets the DijkstraSearch state. Must be called before every search.
 */
void dijkstra_before_search(DijkstraSearch* s, float max_cost, unsigned short max_length);

/**
 * Actually performs the Dijkstra search to find the optimal path.
 *
 * dijkstra_before_search must be called beforehand.
 */
DijkstraSearchResult dijkstra_search(DijkstraSearch* s, unsigned short start, unsigned short end);

/**
 * Compares 2 nodes of the DijkstraSeach.
 */
int dijkstra_compare_entries(DijkstraSearch*, SearchNode const*, SearchNode const*);

/**
 * Internal callback function after search->queue_data[i] has been swapped with
 * search->queue_data[j].
 */
void dijkstra_after_swap(DijkstraSearch*, size_t, size_t);

/**
 * Internal function used to reconstruct a path to `end`.
 */
static void dijkstra_reconstruct_path(DijkstraSearch* s, DijkstraSearchResult* into,
                                      SearchNode end);
