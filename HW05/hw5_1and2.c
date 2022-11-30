#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "hw5_common.h"

// Program state

static Point points[MAX_POINTS_LEN];
static unsigned points_len;

static float edge_costs[MAX_POINTS_LEN][MAX_POINTS_LEN];

// Priority-search data structures

typedef struct {
    unsigned short point_index;
    unsigned short nodes_visited;
} Node;

typedef struct {
    Node nd;  // NOTE: nd must be first to ensure (Entry*) can be casted to (Node*)
    float total_cost;
    unsigned queue_index;
} Entry;

// Priority search related state

#define INVALID_INDEX UINT_MAX
#define NO_MAX_LEN USHRT_MAX
#define QUEUE_CAPACITY (MAX_POINTS_LEN * MAX_POINTS_LEN)

static_assert(QUEUE_CAPACITY < UINT_MAX, "unsigned int will be used to index the queue");

static float max_cost;

#ifdef MAX_LEN
static unsigned short max_length = MAX_LEN;
#else
static unsigned short max_length = NO_MAX_LEN;
#endif

static void* queue_data[QUEUE_CAPACITY];
static Entry entries[MAX_POINTS_LEN][MAX_POINTS_LEN];
static Node previous[MAX_POINTS_LEN][MAX_POINTS_LEN];

static struct {
    float total_cost;
    unsigned short length;
    unsigned short route[MAX_POINTS_LEN];
} solution;

// Priority search

static inline Entry* get_entry(Node n) { return &entries[n.point_index][n.nodes_visited]; }
static inline Node get_previous(Node to) { return previous[to.point_index][to.nodes_visited]; }
static inline void set_previous(Node to, Node from) {
    previous[to.point_index][to.nodes_visited] = from;
}

int compare_entries(Entry const* a, Entry const* b) {
    // To ensure we get the best candidate in main Dijkstra loop, we need
    // the comparison to prefer the following properties, in order:
    // 1. More nodes can be visited
    // 2. More nodes have already been visited
    // 3. Less fuel was used
    //
    // Since we'll be implementing a min-heap; this function should
    // return -1 if a is better than b; 1 if b is better than a or 0 if there are the same.
    unsigned a_to_visit = points_len - a->nd.point_index - 1;
    unsigned b_to_visit = points_len - b->nd.point_index - 1;

    if (a_to_visit == b_to_visit && a->nd.nodes_visited == b->nd.nodes_visited) {
        // Case 3: compare used fuel
        return compare_float_directly(a->total_cost, b->total_cost);
    } else if (a_to_visit == b_to_visit) {
        // Case 2: compare visited nodes
        return compare_int_directly(b->nd.nodes_visited, a->nd.nodes_visited);
    } else {
        // Case 1: compare nodes that can be (potentially) visited
        return compare_int_directly(b_to_visit, a_to_visit);
    }
}

void on_entry_index_update(Entry* entry, size_t new_index) {
    if (new_index == (size_t)-1)
        entry->queue_index = INVALID_INDEX;
    else {
        assert(new_index < UINT_MAX);
        entry->queue_index = new_index;
    }
}

void set_solution(Node end) {
    solution.total_cost = get_entry(end)->total_cost;
    solution.length = end.nodes_visited;
    do {
        solution.route[end.nodes_visited - 1] = end.point_index;
        end = get_previous(end);
    } while (end.nodes_visited > 0);
}

void reset_priority_search_state(void) {
    for (unsigned short i = 0; i < MAX_POINTS_LEN; ++i) {
        for (unsigned short j = 0; j < MAX_POINTS_LEN; ++j) {
            entries[i][j] = (Entry){
                .nd = {.point_index = i, .nodes_visited = j},
                .queue_index = INVALID_INDEX,
                .total_cost = INFINITY,
            };
        }
    }
}

void priority_search_solution(void) {
    // Prepare data structures
    reset_priority_search_state();
    Heap queue = {
        .data = queue_data,
        .capacity = QUEUE_CAPACITY,
        .length = 0,
        .compare_elements = (heap_comparator)compare_entries,
        .on_index_update = (heap_on_index_update)on_entry_index_update,
    };

    // Set start node cost to zero and push into the queue
    entries[0][1].total_cost = 0.0;
    heap_push(&queue, &entries[0][1]);

    // Loop while there are elements
    while (queue.length > 0) {
        Entry* popped = heap_pop(&queue, 0);

        // End reached
        if (popped->nd.point_index == points_len - 1 &&
            (max_length == NO_MAX_LEN || popped->nd.nodes_visited == max_length))
            return set_solution(popped->nd);

        // Add adjacent nodes
        // NOTE: Assumes that the points are sorted
        for (unsigned short next_idx = popped->nd.point_index + 1; next_idx < points_len;
             ++next_idx) {
            Node next_nd = {.point_index = next_idx,
                            .nodes_visited = popped->nd.nodes_visited + 1};
            Entry* next = get_entry(next_nd);

            // Skip edges not permitted by the max_length limit
            if (max_length != NO_MAX_LEN && next->nd.nodes_visited > max_length) continue;

            // Calculate the alternative cost of getting to the next_nd
            float alt_cost = popped->total_cost + edge_costs[popped->nd.point_index][next_idx];

            // Skip edges for which we have a cheaper route
            if (alt_cost > next->total_cost) continue;

            // Skip edges not permitted by the max_fuel limit
            if (alt_cost > max_cost) continue;

            // Remember that the route to `next_nd` via `popped` is better
            next->total_cost = alt_cost;
            set_previous(next_nd, popped->nd);

            // Push the Node into queue, or, if already exists, sift it down
            if (next->queue_index != INVALID_INDEX) {
                // Entry already in the queue - restore heap invariant after the cost has decreased
                heap_sift_down(&queue, next->queue_index);
            } else {
                // No entry to replace - push into queue
                heap_push(&queue, next);
            }
        }
    }

    // No solution
    solution.total_cost = INFINITY;
    solution.length = 0;
    return;
}

// Main entry point

static float max_costs[] = {29.0, 45.0, 77.0, 150.0};  // for 20 points
// static float max_costs[] = {300.0, 450.0, 850.0, 1150.0};  // for 100 points

static float max_costs_len = sizeof(max_costs) / sizeof(max_costs[0]);

void calculate_edge_costs(void) {
    for (unsigned from = 0; from < points_len; ++from) {
        for (unsigned to = 0; to < points_len; ++to) {
            if (from < to)
                edge_costs[from][to] = distance_between(points[from], points[to]);
            else
                edge_costs[from][to] = INFINITY;
        }
    }
}

int main(int argc, char** argv) {
    // Get the input file
    FILE* input = NULL;
    if (argc < 2) {
        // No arguments - read from standard input
        input = stdin;
    } else if (argc == 2) {
        // Single argument - file name
        input = fopen(argv[1], "r");
        if (!input) {
            perror("fopen");
            exit(1);
        }
    } else {
        exit_with_message("Usage: ./hw5 [input_file.txt]");
    }

    // Load the inputs
    load_points(input, points, &points_len);
    calculate_edge_costs();

    // Do the searches
    for (size_t i = 0; i < max_costs_len; ++i) {
        max_cost = max_costs[i];

        clock_t elapsed = clock();
        priority_search_solution();
        elapsed = clock() - elapsed;

        // Print the solution
        printf("%.0f %.1f (%hu points)\n", max_costs[i], solution.total_cost, solution.length);
        for (size_t i = 0; i < solution.length; ++i) {
            Point pt = points[solution.route[i]];
            printf("%.0f %.0f\t", pt.x, pt.y);
        }
        fputc('\n', stdout);
        printf("%.5f seconds\n\n", (float)elapsed / CLOCKS_PER_SEC);
    }
}
