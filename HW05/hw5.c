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

// Helper functions

/**
 * exit_with_message prints the provided message to stderr followed by a newline,
 * then terminates the program by calling exit
 */
noreturn void exit_with_message(char const* msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

// Point structure and Point-related functions

/**
 * The expected number of points
 */
#define MAX_POINTS_COUNT 100

static_assert(MAX_POINTS_COUNT < USHRT_MAX, "unsigned short is used to index points");

typedef struct {
    float x;
    float y;
} Point;

/**
 * All loaded points into the program
 */
static Point points[MAX_POINTS_COUNT] = {0};

/**
 * Number of all loaded points into the program
 */
static unsigned points_len = 0;

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

float distance(Point a, Point b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return hypotf(dx, dy);
}

/**
 * Loads points from a given file, and verifies that provided data matches
 * assignments constraints.
 */
void load_points(FILE* f) {
    // Load point count and verify it
    if (fscanf(f, "%u", &points_len) != 1)
        exit_with_message("Failed to load point count from input file");
    if (points_len > MAX_POINTS_COUNT) exit_with_message("Too many points in input file");

    // Load the points
    for (size_t i = 0; i < points_len; ++i) {
        if (fscanf(f, "%f %f", &points[i].x, &points[i].y) != 2)
            exit_with_message("Failed to load point from file");
    }

    // Sort points (needed for fast evaluation of whether to add a node to a queue)
    qsort(points, points_len, sizeof(Point), compare_points);

    // Ensure first point is (0, 0)
    if (points[0].x != 0.0 || points[0].y != 0.0) exit_with_message("First point must be (0, 0)");

    // Ensure last point is (100, 100)
    // if (points[points_len - 1].x != 100.0 || points[points_len - 1].y != 100.0)
    // exit_with_message("Last point must be (100, 100)");
}

// Data structures for (modified) Dijkstra algorithm

typedef struct {
    unsigned short pt;
    unsigned short nodes_visited;
} SearchEntryState;

typedef struct {
    SearchEntryState state;
    float used_fuel;
} SearchEntry;

#define CACHED_ENTRY_INVALID 0
#define CACHED_ENTRY_IN_QUEUE 1
#define CACHED_ENTRY_VISITED 2

typedef struct {
    unsigned char type;
    union {
        unsigned q_idx;
        float used_fuel;
    };
} CachedEntryIndex;

int compare_search_entries(SearchEntry const* a, SearchEntry const* b) {
    // To ensure we get the best candidate in main Dijkstra loop, we need
    // the comparison to prefer the following properties, in order:
    // 1. More nodes can be visited
    // 2. More nodes have already been visited
    // 3. Less fuel was used
    //
    // Since we'll be implementing a min-heap; this function should
    // return -1 if a is better than b; 1 if b is better than a or 0 if there are the same.
    unsigned a_to_visit = points_len - a->state.pt - 1;
    unsigned b_to_visit = points_len - b->state.pt - 1;

    if (a_to_visit == b_to_visit && a->state.nodes_visited == b->state.nodes_visited) {
        // Case 3: compare used fuel
        return (a->used_fuel > b->used_fuel) - (a->used_fuel < b->used_fuel);
    } else if (a_to_visit == b_to_visit) {
        // Case 2: compare visited nodes
        return (a->state.nodes_visited < b->state.nodes_visited) -
               (a->state.nodes_visited > b->state.nodes_visited);
    } else {
        // Case 1: compare nodes that can be (potentially) visited
        return (a_to_visit < b_to_visit) - (a_to_visit > b_to_visit);
    }
}

// Dijkstra algorithm state

/// Mapping from (state.pt, state.nodes_visited) → (previous_state)
/// for reconstructing the path used.
static SearchEntryState previous_mapping[MAX_POINTS_COUNT][MAX_POINTS_COUNT];

#define QUEUE_CAPACITY (MAX_POINTS_COUNT * MAX_POINTS_COUNT)
static_assert(QUEUE_CAPACITY < UINT_MAX, "unsigned int is used to index the queue");

/// Priority queue (binary heap) for getting the best candidate to expand.
static SearchEntry queue[QUEUE_CAPACITY];
static unsigned queue_len = 0;

/// Mapping from (state.pt, state.nodes_visited) → index into queue.
/// Used for comparing candidates to push into the queue with
/// elements already existing in the queue.
static CachedEntryIndex entry_indices_into_queue[MAX_POINTS_COUNT][MAX_POINTS_COUNT];

/// The solution path - list of point indices
static unsigned short solution[MAX_POINTS_COUNT];

static inline void update_entry_index(SearchEntryState s, unsigned new_index) {
    entry_indices_into_queue[s.pt][s.nodes_visited].type = CACHED_ENTRY_IN_QUEUE;
    entry_indices_into_queue[s.pt][s.nodes_visited].q_idx = new_index;
}

static inline void remove_entry_index(SearchEntryState s, float fuel_used) {
    entry_indices_into_queue[s.pt][s.nodes_visited].type = CACHED_ENTRY_VISITED;
    entry_indices_into_queue[s.pt][s.nodes_visited].q_idx = fuel_used;
}

static inline CachedEntryIndex get_entry_index(SearchEntryState s) {
    return entry_indices_into_queue[s.pt][s.nodes_visited];
}

// State initialization code

void initialize_entry_indices_into_queue(void) {
#ifndef NDEBUG
    for (size_t i = 0; i < MAX_POINTS_COUNT; ++i) {
        for (size_t j = 0; j < MAX_POINTS_COUNT; ++j) {
            entry_indices_into_queue[i][j].type = CACHED_ENTRY_INVALID;
        }
    }
#else
    // Setting all the bits should be a bit faster :^)
    memset(entry_indices_into_queue, 0, sizeof(entry_indices_into_queue));
#endif
}

void reset_dijkstra_state(void) {
    initialize_entry_indices_into_queue();
    queue_len = 0;

#ifndef NDEBUG
    // The program doesn't rely on any initial state of the following arrays.
    // Clear them only when debugging.
    memset(previous_mapping, 0, sizeof(previous_mapping));
    memset(queue, 0, sizeof(queue));
    memset(solution, 0, sizeof(solution));
#endif
}

// Priority queue implementation

void queue_sift_down(unsigned index) {
    assert(index < queue_len);

    unsigned left = 2 * index + 1;
    unsigned right = left + 1;

    // Find the smallest node
    unsigned smallest = index;

    if (left < queue_len && compare_search_entries(queue + left, queue + smallest) < 0)
        smallest = left;

    if (right < queue_len && compare_search_entries(queue + right, queue + smallest) < 0)
        smallest = right;

    // If the smallest isn't root - swap and recurse downwards
    if (smallest != index) {
        SearchEntry old_root = queue[index];
        SearchEntry new_root = queue[smallest];

#ifndef NDEBUG
        CachedEntryIndex old_root_ce = get_entry_index(old_root.state);
        assert(old_root_ce.type == CACHED_ENTRY_IN_QUEUE);
        assert(old_root_ce.q_idx == index);

        CachedEntryIndex new_root_ce = get_entry_index(new_root.state);
        assert(new_root_ce.type == CACHED_ENTRY_IN_QUEUE);
        assert(new_root_ce.q_idx == smallest);
#endif

        queue[smallest] = old_root;
        update_entry_index(old_root.state, smallest);

        queue[index] = new_root;
        update_entry_index(new_root.state, index);

        queue_sift_down(smallest);
    }
}

void queue_sift_up(unsigned index) {
    assert(index < queue_len);

    SearchEntry inserted = queue[index];
    unsigned parent_idx = (index - 1) / 2;

    while (index > 0 && compare_search_entries(&inserted, queue + parent_idx) < 0) {
        queue[index] = queue[parent_idx];
        update_entry_index(queue[index].state, index);

        index = parent_idx;
        parent_idx = (index - 1) / 2;
    }

    queue[index] = inserted;
    update_entry_index(queue[index].state, index);
}

SearchEntry queue_pop(unsigned index) {
    assert(index < queue_len);

    SearchEntry removed = queue[index];
    remove_entry_index(removed.state, removed.used_fuel);

    --queue_len;

    // No swaps required when removing the last item
    if (index != queue_len) {
        queue[index] = queue[queue_len];
        update_entry_index(queue[index].state, index);
        queue_sift_down(index);
    }

    return removed;
}

void queue_push(SearchEntry new_entry) {
    assert(queue_len < QUEUE_CAPACITY);
    unsigned index = queue_len++;

    queue[index] = new_entry;
    update_entry_index(new_entry.state, index);

    queue_sift_up(index);
}

// Dijkstra algorithm implementation

void reconstruct_path(SearchEntryState end) {
#ifndef NDEBUG
    // When debugging - ensure we have inserted precisely end.nodes_visited nodes
    unsigned expected_len = end.nodes_visited;
    unsigned inserted = 1;
#endif

    solution[end.nodes_visited - 1] = end.pt;

    while (end.nodes_visited > 1) {
        end = previous_mapping[end.pt][end.nodes_visited];

#ifndef NDEBUG
        ++inserted;
#endif

        assert(end.nodes_visited > 0);
        solution[end.nodes_visited - 1] = end.pt;
    }

#ifndef NDEBUG
    assert(inserted == expected_len);
#endif
}

SearchEntry find_solution(unsigned short start, unsigned short end, float max_fuel) {
    reset_dijkstra_state();

    // Push the start entry
    queue_push((SearchEntry){.state = {.pt = start, .nodes_visited = 1}, .used_fuel = 0.0});

    while (queue_len > 0) {
        SearchEntry popped = queue_pop(0);

        if (popped.state.pt == end) {
            reconstruct_path(popped.state);
            return popped;
        }

        // NOTE: Thanks to points being sorted: from point `i` we can only reach i+1, i+2, ...
        for (unsigned short neighbor = popped.state.pt + 1; neighbor < points_len; ++neighbor) {
            SearchEntry new_entry = {
                .state = {.pt = neighbor, .nodes_visited = popped.state.nodes_visited + 1},
                .used_fuel =
                    popped.used_fuel + distance(points[popped.state.pt], points[neighbor]),
            };

            // Fuel capacity exceeded - don't push
            if (new_entry.used_fuel > max_fuel) continue;

            // See if the (point, nodes_visited) pair has already been expanded
            // and whether it should be expanded further
            CachedEntryIndex existing_entry = get_entry_index(new_entry.state);
            switch (existing_entry.type) {
                case CACHED_ENTRY_INVALID:
                    // First time seeing this state
                    break;

                case CACHED_ENTRY_IN_QUEUE:
                    // This state is already in the queue -
                    // see if `new_entry` should replace it
                    if (new_entry.used_fuel < queue[existing_entry.q_idx].used_fuel) {
                        // `new_entry` uses less fuel - keep it
                        queue_pop(existing_entry.q_idx);
                    } else {
                        // existing entry uses less fuel - abandon `new_entry`
                        continue;
                    }
                    break;

                case CACHED_ENTRY_VISITED:
                    // State was already visited and expanded -
                    // assume existing entry was better
                    assert(existing_entry.used_fuel <= new_entry.used_fuel);
                    continue;
            }

            previous_mapping[neighbor][new_entry.state.nodes_visited] = popped.state;
            queue_push(new_entry);
        }
    }

    return (SearchEntry){0};
}

// Entry point

static float max_fuels[] = {300.0f, 450.0f, 850.0f, 1150.0f, 0.0f};

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

    // Load the input
    load_points(input);

    // Run the solutions
    for (float* max_fuel = max_fuels; *max_fuel; ++max_fuel) {
        clock_t elapsed = clock();

        // Find the solution
        SearchEntry best = find_solution(0, points_len - 1, *max_fuel);

        elapsed = clock() - elapsed;

        // Print the solution
        printf("%.0f %.1f (%hu points)\n", *max_fuel, best.used_fuel, best.state.nodes_visited);
        for (size_t i = 0; i < best.state.nodes_visited; ++i) {
            printf("%.0f %.0f\t", points[solution[i]].x, points[solution[i]].y);
        }
        fputc('\n', stdout);

        printf("%.5f seconds\n\n", (float)elapsed / CLOCKS_PER_SEC);
    }

    return 0;
}
