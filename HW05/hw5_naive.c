#include <assert.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>

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
 * Distances between given pair of points
 */
static float point_distances[MAX_POINTS_COUNT][MAX_POINTS_COUNT];

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

void recalculate_point_distances(void) {
    for (unsigned from = 0; from < points_len; ++from) {
        for (unsigned to = 0; to < points_len; ++to) {
            if (from > to) {
                point_distances[from][to] = INFINITY;
            } else if (from == to) {
                point_distances[from][to] = 0;
            } else {
                point_distances[from][to] = distance(points[from], points[to]);
            }
        }
    }
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

    recalculate_point_distances();
}

// Combination generator

typedef struct {
    unsigned pool_length;
    unsigned combination_length;
    unsigned index_offset;

    unsigned indices[0];
} Combinator;

void initialize_combinator(Combinator* c, unsigned combination_length) {
    c->combination_length = combination_length;
    for (unsigned i = 0; i < c->combination_length; ++i) c->indices[i] = i + c->index_offset;
}

Combinator* new_combinator(unsigned pool_length, unsigned combination_length) {
    if (combination_length > pool_length) return NULL;

    Combinator* c = malloc(sizeof(Combinator) + sizeof(unsigned) * combination_length);
    c->index_offset = 0;
    c->pool_length = pool_length;
    initialize_combinator(c, combination_length);
    return c;
}

bool next_combination(Combinator* c) {
    // Source: https://docs.python.org/3/library/itertools.html#itertools.combinations

    unsigned i = c->combination_length - 1;
    for (; i != UINT_MAX; --i) {
        if (c->indices[i] - c->index_offset != i + c->pool_length - c->combination_length) break;
    }

    // No new combination
    if (i == UINT_MAX) return false;

    ++c->indices[i];
    for (unsigned j = i + 1; j < c->combination_length; ++j) c->indices[j] = c->indices[j - 1] + 1;

    return true;
}

bool next_combination_regardless_of_length(Combinator* c) {
    bool has_of_same_length = next_combination(c);

    if (has_of_same_length) return true;
    if (c->combination_length >= c->pool_length) return false;

    initialize_combinator(c, c->combination_length + 1);
    return true;
}

void on_all_combinations(unsigned len, void (*cb)(unsigned*, unsigned)) {
    Combinator* c = new_combinator(len, len);
    initialize_combinator(c, 0);
    do {
        cb(c->indices, c->combination_length);
    } while (next_combination_regardless_of_length(c));
    free(c);
}

// Naive search

typedef struct {
    unsigned points[MAX_POINTS_COUNT];
    unsigned length;
    float used_fuel;
} Guess;

void replace_guess_if_better(Guess* dst, Guess const* src) {
    if (src->length > dst->length ||
        (src->length == dst->length && src->used_fuel > dst->used_fuel)) {

        memcpy(dst, src, sizeof(Guess));
    }
}

typedef struct {
    float const max_fuel;

    Combinator* c;
    bool done;
    pthread_mutex_t c_mtx;
    pthread_cond_t done_cond;

    Guess best;
    pthread_mutex_t best_mtx;
} NaiveSearchState;

bool nss_next_route(NaiveSearchState* nss, unsigned* through_indices, unsigned* through_len) {
    pthread_mutex_lock(&nss->c_mtx);
    bool has_next_combination = false;
    if (!nss->done) {
        has_next_combination = next_combination_regardless_of_length(nss->c);
        if (has_next_combination) {
            *through_len = nss->c->combination_length;
            memcpy(through_indices, nss->c->indices,
                   sizeof(unsigned) * nss->c->combination_length);
        } else {
            nss->done = true;
            pthread_cond_broadcast(&nss->done_cond);
        }
    }

    pthread_mutex_unlock(&nss->c_mtx);
    return has_next_combination;
}

void nss_maybe_update_best(NaiveSearchState* nss, Guess const* guess) {
    pthread_mutex_lock(&nss->best_mtx);
    replace_guess_if_better(&nss->best, guess);
    pthread_mutex_unlock(&nss->best_mtx);
}

void* nss_worker(void* arg) {
    NaiveSearchState* nss = arg;
    unsigned through_len;
    unsigned through[MAX_POINTS_COUNT];

    while (nss_next_route(nss, through, &through_len)) {
        Guess guess;
        guess.length = through_len + 2;
        guess.used_fuel = 0.0f;

        // Initialize guess points
        guess.points[0] = 0;
        memcpy(guess.points + 1, through, sizeof(unsigned) * through_len);
        guess.points[guess.length - 1] = points_len - 1;

        // Calculate the total used fuel
        bool admissable = true;
        for (unsigned i = 1; admissable && i < guess.length; ++i) {
            guess.used_fuel += point_distances[guess.points[i - 1]][guess.points[i]];
            admissable = guess.used_fuel < nss->max_fuel;
        }

        // Fuel cap exceeded - move on to next combination
        if (!admissable) continue;

        nss_maybe_update_best(nss, &guess);
    }

    return NULL;
}

void nss_init(NaiveSearchState* nss) {
    nss->c_mtx = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    nss->best_mtx = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    nss->done_cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;

    nss->c = new_combinator(points_len - 2, points_len - 2);
    nss->c->index_offset = 1;
    initialize_combinator(nss->c, 0);

    nss->best.length = 2;
    nss->best.points[0] = 0;
    nss->best.points[1] = points_len - 1;
    nss->best.used_fuel = point_distances[0][points_len - 1];
}

void nss_free(NaiveSearchState* nss) {
    pthread_mutex_destroy(&nss->c_mtx);
    pthread_mutex_destroy(&nss->best_mtx);
    free(nss->c);
}

Guess parallel_naive_search(float max_fuel) {
    int cpus = sysconf(_SC_NPROCESSORS_ONLN);

    // Initialize search state
    NaiveSearchState nss = {.max_fuel = max_fuel};
    nss_init(&nss);

    // Spawn workers
    pthread_t workers[cpus];
    for (int i = 0; i < cpus; ++i) pthread_create(workers + i, 0, nss_worker, &nss);

    // Wait until they are done
    pthread_mutex_lock(&nss.c_mtx);
    while (!nss.done) pthread_cond_wait(&nss.done_cond, &nss.c_mtx);
    pthread_mutex_unlock(&nss.c_mtx);

    // Join all threads
    for (int i = 0; i < cpus; ++i) pthread_join(workers[i], NULL);

    // Release resources
    nss_free(&nss);

    return nss.best;
}

// Main program

void print_unsigned_arr(unsigned* arr, unsigned len) {
    printf("{ ");
    for (unsigned i = 0; i < len; ++i) {
        printf("%u ", arr[i]);
    }
    printf("}\n");
}

// static float max_fuels[] = {300.0f, 450.0f, 850.0f, 1150.0f, 0.0f};
static float max_fuels[] = {29.0f, 45.0f, 77.0f, 150.0f, 0.0f};


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
        Guess best = parallel_naive_search(*max_fuel);

        elapsed = clock() - elapsed;

        // Print the solution
        printf("%.0f %.1f (%hu points)\n", *max_fuel, best.used_fuel, best.length);
        for (size_t i = 0; i < best.length; ++i) {
            printf("%.0f %.0f\t", points[best.points[i]].x, points[best.points[i]].y);
        }
        fputc('\n', stdout);
        printf("%.5f seconds\n\n", (float)elapsed / CLOCKS_PER_SEC);
    }

    return 0;
}
