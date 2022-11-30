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

static Node queue_data[QUEUE_CAPACITY];
static unsigned entry_indices[MAX_POINTS_LEN][MAX_POINTS_LEN];
static float total_costs[MAX_POINTS_LEN][MAX_POINTS_LEN];
static Node previous[MAX_POINTS_LEN][MAX_POINTS_LEN];

static struct {
    float total_cost;
    unsigned short length;
    unsigned short route[MAX_POINTS_LEN];
} solution;

// Priority search

static inline unsigned get_entry_index(Node n) {
    return entry_indices[n.point_index][n.nodes_visited];
}
static inline void set_entry_index(Node n, unsigned v) {
    entry_indices[n.point_index][n.nodes_visited] = v;
}

static inline float get_total_cost(Node n) { return total_costs[n.point_index][n.nodes_visited]; }
static inline void set_total_cost(Node n, float v) {
    total_costs[n.point_index][n.nodes_visited] = v;
}

static inline Node get_previous(Node to) { return previous[to.point_index][to.nodes_visited]; }
static inline void set_previous(Node to, Node from) {
    previous[to.point_index][to.nodes_visited] = from;
}

int compare_nodes(Node const* a, Node const* b) {
    // To ensure we get the best candidate in main Dijkstra loop, we need
    // the comparison to prefer the following properties, in order:
    // 1. More nodes can be visited
    // 2. More nodes have already been visited
    // 3. Less fuel was used
    //
    // Since we'll be implementing a min-heap; this function should
    // return -1 if a is better than b; 1 if b is better than a or 0 if there are the same.
    unsigned a_to_visit = points_len - a->point_index - 1;
    unsigned b_to_visit = points_len - b->point_index - 1;

    if (a_to_visit == b_to_visit && a->nodes_visited == b->nodes_visited) {
        // Case 3: compare used fuel
        return compare_float_directly(get_total_cost(*a), get_total_cost(*b));
    } else if (a_to_visit == b_to_visit) {
        // Case 2: compare visited nodes
        return compare_int_directly(b->nodes_visited, a->nodes_visited);
    } else {
        // Case 1: compare nodes that can be (potentially) visited
        return compare_int_directly(b_to_visit, a_to_visit);
    }
}

void after_swap(size_t i, size_t j) {
    set_entry_index(queue_data[i], i);
    set_entry_index(queue_data[j], j);
}

void set_solution(Node end) {
    solution.total_cost = get_total_cost(end);
    solution.length = end.nodes_visited;
    do {
        solution.route[end.nodes_visited - 1] = end.point_index;
        end = get_previous(end);
    } while (end.nodes_visited > 0);
}

void reset_priority_search_state(void) {
    for (unsigned short i = 0; i < MAX_POINTS_LEN; ++i) {
        for (unsigned short j = 0; j < MAX_POINTS_LEN; ++j) {
            entry_indices[i][j] = INVALID_INDEX;
            total_costs[i][j] = INFINITY;
        }
    }
}

void priority_search_solution(void) {
    // Prepare data structures
    reset_priority_search_state();
    Heap queue = {
        .data = queue_data,
        .capacity = QUEUE_CAPACITY,
        .element_size = sizeof(Node),
        .length = 0,
        .compare_elements = (heap_comparator)compare_nodes,
        .after_swap = after_swap,
    };

    // Push the first point
    Node start = {.point_index = 0, .nodes_visited = 1};
    set_entry_index(start, 0);
    set_total_cost(start, 0.0);
    set_previous(start, (Node){0});
    heap_push(&queue, &start);

    // Loop while there are elements
    while (queue.length > 0) {
        Node popped;
        heap_pop(&queue, &popped, 0);
        set_entry_index(popped, INVALID_INDEX);

        // End reached
        if (popped.point_index == points_len - 1 &&
            (max_length == NO_MAX_LEN || popped.nodes_visited == max_length))
            return set_solution(popped);

        // Add adjacent nodes
        for (unsigned short next_idx = popped.point_index + 1; next_idx < points_len; ++next_idx) {
            Node next = {.point_index = next_idx, .nodes_visited = popped.nodes_visited + 1};

            // Skip edges not permitted by the max_length limit
            if (max_length != NO_MAX_LEN && next.nodes_visited > max_length) continue;

            float next_cost = get_total_cost(popped) + edge_costs[popped.point_index][next_idx];

            // Skip edges for which we have a cheaper route
            if (next_cost > get_total_cost(next)) continue;

            // Skip edges not permitted by the max_fuel limit
            if (next_cost > max_cost) continue;

            // Update helper data structures
            set_total_cost(next, next_cost);
            set_previous(next, popped);

            // Push the Node into queue, or, if already exists, sift it down
            unsigned existing_idx = get_entry_index(next);
            if (existing_idx != INVALID_INDEX) {
                // There's an entry to replace
                heap_sift_down(&queue, existing_idx);
            } else {
                // No entry to replace - push into queue
                set_entry_index(next, queue.length);
                heap_push(&queue, &next);
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
