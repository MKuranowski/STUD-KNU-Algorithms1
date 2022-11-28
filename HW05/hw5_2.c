#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "hw5_common.h"
#include "hw5_oneway_dijkstra.h"

// Program state

static Point points[MAX_POINTS_LEN];
static unsigned points_len;

static float edge_costs[MAX_POINTS_LEN][MAX_POINTS_LEN];

static DijkstraSearch search;

static float max_costs[] = {29.0, 45.0, 77.0, 150.0};
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

float get_edge_cost(void* context, SearchNode from, SearchNode to) {
    (void)context;
    return edge_costs[from.point_index][to.point_index];
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

    // Initialize dijkstra search
    dijkstra_init(&search, points_len, (dijkstra_edge_getter)get_edge_cost, NULL);

    // Do the searches
    for (size_t i = 0; i < max_costs_len; ++i) {
        clock_t elapsed = clock();

        dijkstra_before_search(&search, max_costs[i], DIJKSTRA_NO_MAX_LEN);
        DijkstraSearchResult result = dijkstra_search(&search, 0, points_len - 1);

        elapsed = clock() - elapsed;

        // Print the solution
        printf("%.0f %.1f (%hu points)\n", max_costs[i], result.cost, result.route_length);
        for (size_t i = 0; i < result.route_length; ++i) {
            Point pt = points[result.route[i]];
            printf("%.0f %.0f\t", pt.x, pt.y);
        }
        fputc('\n', stdout);
        printf("%.5f seconds\n\n", (float)elapsed / CLOCKS_PER_SEC);
    }
}
