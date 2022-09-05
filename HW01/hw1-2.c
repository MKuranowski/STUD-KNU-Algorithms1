/*
 * ID: 2020427681
 * NAME: Mikolaj Kuranowski
 * OS: MacOS 12.5.1
 * Compiler version: clang 13.1.6
 */

#include <stdio.h>
#include <stdlib.h>

int compare_ints(void const* a_ptr, void const* b_ptr) {
    int a = *(int const*)a_ptr;
    int b = *(int const*)b_ptr;
    return (a > b) - (b > a);
}

int main(int argc, char** argv) {
    // Check the arguments
    if (argc != 4) {
        fputs("Usage: ./hw1-2 number_1 number_2 number_3\n", stderr);
        return 1;
    }

    // Convert the arguments to numbers
    int numbers[3];
    numbers[0] = atoi(argv[1]);
    numbers[1] = atoi(argv[2]);
    numbers[2] = atoi(argv[3]);

    // Sort the list
    qsort(numbers, 3, sizeof(int), compare_ints);

    // Print the result
    printf("%d %d %d\n", numbers[0], numbers[1], numbers[2]);

    return 0;
}
