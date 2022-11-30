/*
 * ID: 2020427681
 * NAME: Mikolaj Kuranowski
 * OS: Debian 11
 * Compiler version: gcc 12.2.0
 */

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <time.h>

/**
 * @defgroup Helpers
 *
 * This group contains generic helper functions
 * used throughout the whole program.
 *
 * @{
 */

/**
 * Prints the provided message to stderr followed by a newline,
 * then terminates the program by calling exit.
 */
noreturn void exit_with_message(char const* msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

/**
 * CMP is a generic macro, which evaluates to:
 * - `1` if a > b,
 * - `0` if a == b,
 * - `-1` if a < b.
 *
 * The macro arguments are evaluated twice.
 */
#define CMP(a, b) (((a) > (b)) - ((a) < (b)))

/**@}*/
/**
 * @defgroup Points
 *
 * This group defines the Point structure and
 * function related to handling them.
 *
 * @{
 */

/**
 * Point represents a tuple of 2 floats
 */
typedef struct {
    float x;
    float y;
} Point;

/**
 * The expected number of points
 */
#define MAX_POINTS_LEN 100

static_assert(MAX_POINTS_LEN < USHRT_MAX, "unsigned short is used to index points");

/**
 * Compares 2 points lexicographically.
 */
int compare_points(void const* a_ptr, void const* b_ptr) {
    Point const* a = a_ptr;
    Point const* b = b_ptr;

    if (a->x == b->x) {
        return (a->y > b->y) - (a->y < b->y);
    } else {
        return (a->x > b->x) - (a->x < b->x);
    }
}

/**
 * Calculates the Euclidean distance between 2 points.
 */
static inline float distance_between(Point a, Point b) { return hypotf(a.x - b.x, a.y - b.y); }

/**
 * Loads points from a given file.
 */
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

/**@}*/
/**
 * @defgroup Heap
 *
 * This group defines a binary min-heap of pointers
 * and operations on the heap.
 *
 * @{
 */

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
 * Helper function to swap elements i and j in the heap, and call appropiate callbacks.
 */
static inline void heap_swap(Heap* h, size_t i, size_t j) {
    assert(i != j);
    assert(i < h->length);
    assert(j < h->length);

    void* tmp = h->data[i];
    h->data[i] = h->data[j];
    h->data[j] = tmp;

    if (h->on_index_update) {
        h->on_index_update(h->data[i], i);
        h->on_index_update(h->data[j], j);
    }
}

/**
 * Helper function to call h->compare_elements with elements at indices i and j.
 */
static inline int heap_cmp(Heap* h, size_t i, size_t j) {
    return h->compare_elements(h->data[i], h->data[j]);
}

/**
 * Moves an element at `index` down until heap invariant is maintained.
 * should be called after heap[index] got its value decreased.
 */
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

/**
 * Moves an element at `index` up until heap invariant is maintained.
 * should be called after heap[index] got its value increased.
 */
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

/**
 * Pushes an element into the MinHeap
 */
void heap_push(Heap* h, void* element) {
    assert(h->length < h->capacity);

    if (h->on_index_update) h->on_index_update(element, h->length);
    h->data[h->length++] = element;

    heap_sift_up(h, h->length - 1);
}

/**
 * Pops `heap[index]` element from the heap into `target`.
 * Use index == 0 to pop the smallest element.
 */
void* heap_pop(Heap* h, size_t index) {
    assert(index < h->length);

    void* popped = h->data[index];

    // No need to restore heap invariant if last element was removed
    if (h->length - 1 != index) {
        heap_swap(h, index, h->length - 1);
        --h->length;
        heap_sift_down(h, index);
    } else {
        --h->length;
    }

    if (h->on_index_update) h->on_index_update(popped, (size_t)-1);
    return popped;
}

/**@}*/
/**
 * @defgroup State
 *
 * This group contains statically allocated state of the program.
 *
 * @{
 */

/**
 * All loaded points, sorted lexicographically.
 */
static Point points[MAX_POINTS_LEN];

/**
 * Number of loaded points.
 */
static unsigned points_len;

/**
 * Precalculated distances between points.
 * INFINITY if it's not possible to traverse an edge.
 */
static float edge_costs[MAX_POINTS_LEN][MAX_POINTS_LEN];

/**@}*/
/**
 * @defgroup Priority Search
 *
 * This group contains structures needed to implement the priority search,
 * statically allocated state of the algorithm and the priority search itself.
 *
 * @{
 */

/**
 * Node literally represents a Vertex in the transformed graph.
 */
typedef struct {
    unsigned short point_index;
    unsigned short nodes_visited;
} Node;

/**
 * Entry represents a candidate to expand in the priority search.
 *
 * There must be at most one Entry corresponding to a given Node.
 *
 * Entry may not necessarily be in the queue (e.g. if the corresponding node has been visited) -
 * in this case `queue_index` is set to INVALID_INDEX.
 */
typedef struct {
    Node nd;
    float total_cost;
    unsigned queue_index;
} Entry;

/**
 * Solution describes the result of priority search algorithm.
 */
typedef struct {
    float total_cost;
    unsigned short length;
    unsigned short route[MAX_POINTS_LEN * 2];  // solution for part 3 we may need more space
} Solution;

/**
 * Flag value for queue_index that represents not-in-queue
 */
#define INVALID_INDEX UINT_MAX

/**
 * Flag value for the search to not limit the number of nodes
 */
#define NO_MAX_LEN USHRT_MAX

#define QUEUE_CAPACITY (MAX_POINTS_LEN * MAX_POINTS_LEN)
static_assert(QUEUE_CAPACITY < UINT_MAX, "unsigned int will be used to index the queue");

/**
 * Statically allocated space for the priority queue data.
 */
static void* queue_data[QUEUE_CAPACITY];

/**
 * Statically allocated space for all possible Entries.
 */
static Entry entries[MAX_POINTS_LEN][MAX_POINTS_LEN];

/**
 * Statically allocated space for all possible previous mappings,
 * for the reconstruction of the solution path.
 */
static Node previous[MAX_POINTS_LEN][MAX_POINTS_LEN];

/**
 * The start and end point of the current search
 */
unsigned short static_start, static_end;

// Priority search

/**
 * Finds an Entry corresponding to a given Node.
 */
static inline Entry* get_entry(Node n) { return &entries[n.point_index][n.nodes_visited]; }

/**
 * Finds the node that should precede given Node.
 */
static inline Node get_previous(Node to) { return previous[to.point_index][to.nodes_visited]; }

/**
 * Updates the previous mapping, denoting that to get to `to` one must go from `from`.
 */
static inline void set_previous(Node to, Node from) {
    previous[to.point_index][to.nodes_visited] = from;
}

/**
 * Compares two Entries, picking which one is better to expand first.
 */
int compare_entries(Entry const* a, Entry const* b) {
    // To ensure we get the best candidate in main priority search loop, we need
    // the comparison to prefer the following properties, in order:
    // 1. More nodes can be visited
    // 2. More nodes have already been visited
    // 3. Less fuel was used
    //
    // Since we'll be implementing a min-heap; this function should
    // return -1 if a is better than b; 1 if b is better than a or 0 if there are the same.
    unsigned a_to_visit = abs((int)static_end - (int)a->nd.point_index);
    unsigned b_to_visit = abs((int)static_end - (int)b->nd.point_index);

    if (a_to_visit == b_to_visit && a->nd.nodes_visited == b->nd.nodes_visited) {
        // Case 3: compare used fuel
        return CMP(a->total_cost, b->total_cost);
    } else if (a_to_visit == b_to_visit) {
        // Case 2: compare visited nodes
        return CMP(b->nd.nodes_visited, a->nd.nodes_visited);
    } else {
        // Case 1: compare nodes that can be (potentially) visited
        return CMP(b_to_visit, a_to_visit);
    }
}

/**
 * Simple handler to set entry->queue_index.
 */
void on_entry_index_update(Entry* entry, size_t new_index) {
    if (new_index == (size_t)-1)
        entry->queue_index = INVALID_INDEX;
    else {
        assert(new_index < UINT_MAX);
        entry->queue_index = new_index;
    }
}

/**
 * Sets `solution` after reaching the end node.
 * The entry in `previous` for the start node must have its node_visited set to 0.
 */
void set_solution(Solution* solution, Node end) {
    solution->total_cost = get_entry(end)->total_cost;
    solution->length = end.nodes_visited;
    do {
        solution->route[end.nodes_visited - 1] = end.point_index;
        end = get_previous(end);
    } while (end.nodes_visited > 0);
}

/**
 * Initializes the priority search state.
 */
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

/**
 * Performs the priority search.
 */
void priority_search_solution(unsigned short start, unsigned short end, float max_cost,
                              unsigned short max_length, Solution* solution) {
    // Set the static_start and static_end for proper nodes_to_visit calculation
    static_start = start;
    static_end = end;

    // Check whether we are performing a forward or backward search
    int expected_direction = CMP(start, end);

    // Prepare data structures
    reset_priority_search_state();
    Heap queue = {
        .data = queue_data,
        .capacity = QUEUE_CAPACITY,
        .length = 0,
        .compare_elements = (heap_comparator)compare_entries,
        .on_index_update = (heap_on_index_update)on_entry_index_update,
    };

    // Initialize start entry to zero and push into the queue
    Entry* start_entry = get_entry((Node){start, 1});
    start_entry->total_cost = 0.0;
    set_previous(start_entry->nd, (Node){0, 0});
    heap_push(&queue, start_entry);

    // Loop while there are elements
    while (queue.length > 0) {
        Entry* popped = heap_pop(&queue, 0);

        // End reached
        if (popped->nd.point_index == end &&
            (max_length == NO_MAX_LEN || popped->nd.nodes_visited == max_length))
            return set_solution(solution, popped->nd);

        // Add adjacent nodes
        for (unsigned short next_idx = 0; next_idx < points_len; ++next_idx) {
            // Check if next_idx is in the direction we're travelling
            if (CMP(popped->nd.point_index, next_idx) != expected_direction) continue;

            // Get the Entry to the next node
            Node next_nd = {next_idx, popped->nd.nodes_visited + 1};
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
    solution->total_cost = INFINITY;
    solution->length = 0;
    return;
}

/**@}*/
/**
 * @defgroup Main
 *
 * This group contains the main entry point of the program
 *
 * @{
 */

/**
 * Pre-calculates the `edge_costs` table.
 */
void calculate_edge_costs(void) {
    for (unsigned from = 0; from < points_len; ++from) {
        for (unsigned to = 0; to < points_len; ++to) {
            edge_costs[from][to] = distance_between(points[from], points[to]);
        }
    }
}

/**
 * Tries to open the input file if provided in argument,
 * or returns stdin if no arguments were provided.
 */
FILE* figure_out_input_file(int argc, char** argv) {
    if (argc < 2) {
        // No arguments - read from standard input
        return stdin;
    } else if (argc == 2) {
        // Single argument - file name
        FILE* input = fopen(argv[1], "r");
        if (!input) {
            perror("fopen");
            exit(1);
        }
        return input;
    } else {
        exit_with_message("Usage: ./hw5 [input_file.txt]");
    }
}

/**
 * Dumps the solution into the sink, as described in the problem PDF.
 * Only prints the max allowed cost if it's normal.
 */
void dump_solution(FILE* sink, Solution* solution, float max_cost, clock_t elapsed) {
    if (isnormal(max_cost)) {
        fprintf(sink, "%.0f %.1f (%hu points)\n", max_cost, solution->total_cost,
                solution->length);
    } else {
        fprintf(sink, "%.1f (%hu points)\n", solution->total_cost, solution->length);
    }

    for (size_t i = 0; i < solution->length; ++i) {
        Point pt = points[solution->route[i]];
        fprintf(sink, "%.0f %.0f\t", pt.x, pt.y);
    }
    fputc('\n', stdout);
    printf("%.5f seconds\n\n", (float)elapsed / CLOCKS_PER_SEC);
}

static float max_costs[] = {300.0, 450.0, 850.0, 1150.0};
static float max_costs_len = sizeof(max_costs) / sizeof(max_costs[0]);

#if PART == 1

void run(void) {
    Solution solution;

    clock_t elapsed = clock();
    priority_search_solution(0, points_len - 1, INFINITY, 30, &solution);
    elapsed = clock() - elapsed;

    dump_solution(stdout, &solution, NAN, elapsed);
}

#elif PART == 2

void run(void) {
    Solution solution;

    for (size_t i = 0; i < max_costs_len; ++i) {
        clock_t elapsed = clock();
        priority_search_solution(0, points_len - 1, max_costs[i], NO_MAX_LEN, &solution);
        elapsed = clock() - elapsed;

        dump_solution(stdout, &solution, max_costs[i], elapsed);
    }
}

#else
#error "PART must be defined and set to either '1' or '2'"
#endif

int main(int argc, char** argv) {
    // Get the input file
    FILE* input = figure_out_input_file(argc, argv);

    // Load the inputs
    load_points(input, points, &points_len);
    calculate_edge_costs();

    // Run the appropiate solver
    run();

    // Close the input file
    if (fclose(input)) {
        perror("fclose");
        exit(1);
    }

    return 0;
}

/**@}*/
