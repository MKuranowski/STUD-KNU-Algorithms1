/*
 * ID: 2020427681
 * NAME: Mikolaj Kuranowski
 * OS: MacOS 12.5.1
 * Compiler version: clang 13.1.6
 */

#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

double find_mean(size_t nums_count, int* nums) {
    double sum = 0.0;
    for (size_t i = 0; i < nums_count; ++i) sum += (double)nums[i];
    return sum / (double)nums_count;
}

double find_variance(size_t nums_count, int* nums, double mean) {
    double sum = 0.0;
    for (size_t i = 0; i < nums_count; ++i) {
        double i_diff = nums[i] - mean;
        sum += i_diff * i_diff;
    }
    return sum / (double)nums_count;
}

int find_min(size_t nums_count, int* nums) {
    int min = INT_MAX;
    for (size_t i = 0; i < nums_count; ++i) {
        if (nums[i] < min) min = nums[i];
    }
    return min;
}

int find_max(size_t nums_count, int* nums) {
    int max = INT_MIN;
    for (size_t i = 0; i < nums_count; ++i) {
        if (nums[i] > max) max = nums[i];
    }
    return max;
}

void load_numbers(char const* filename, size_t* out_nums_count, int** out_nums) {
    // Open the file
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        exit(1);
    }

    // Read the number of numbers from the file
    size_t nums_count = 0;
    fscanf(fp, "%zu", &nums_count);

    // Allocate a vector for the numbers
    int* nums = calloc(nums_count, sizeof(int));

    // Load numbers from the file
    for (size_t i = 0; i < nums_count; i++) {
        fscanf(fp, "%d", nums + i);
    }

    // Close the file
    fclose(fp);

    // Save the output variables
    *out_nums_count = nums_count;
    *out_nums = nums;
}

int main(int argc, char** argv) {
    // Check the arguments
    if (argc != 2) {
        fputs("Usage: ./hw1-3 filename\n", stderr);
        exit(1);
    }

    // Load the numbers
    size_t nums_count = 0;
    int* nums = NULL;
    load_numbers(argv[1], &nums_count, &nums);

    // Find the requested numeric data
    int min = find_min(nums_count, nums);
    int max = find_max(nums_count, nums);
    double mean = find_mean(nums_count, nums);
    double variance = find_variance(nums_count, nums, mean);

    // Print the results
    puts("#data\tmin\tmax\tmean\tvariance");
    printf("%zu\t%d\t%d\t%.1f\t%.1f\n", nums_count, min, max, mean, variance);

    // Free the allocated vector of numbers
    free(nums);

    return 0;
}
