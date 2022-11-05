/* Binary search tree using doubly-linked lists
 * COMP319 Algorithms, Spring 2022
 * School of Electronics Engineering, Kyungpook National University
 * Instructor: Gil-Jin Jang
 */
/* ID: 2020427681
 * NAME: Mikolaj Kuranowski
 * OS: MacOS 12.6
 * Compiler version: clang 14.0.0
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MEASURE_TIME  // to measure time

/////////////////////////////////////////////////////////////
// DATA STRUCTURE:
// binary tree node definition using doubly linked lists
// key is a string of a fixed length
// KEYLENGTH	length of the key
// BULK_SIZE	key is hidden in the "BULK"
//	to retrieve key, we have to dig into the "BULK"
//	so accessing key takes average "BULK_SIZE"/2 searches
//	this is INTENTIONALLY to add extra overhead for search
//	your program should reduce the number of key accesses at your best
/////////////////////////////////////////////////////////////
#define KEYLENGTH 3
#define BULK_SIZE 4096
//#define BULK_SIZE	65536
struct BTNode {
    char bulk[BULK_SIZE];         // null character to be added
    struct BTNode *left, *right;  // binary tree: left and right children
};

/////////////////////////////////////////////////////////////
// GIVEN: functions for binary tree node
// name and parameters only
// implementations are moved after "main" function
/////////////////////////////////////////////////////////////

// return value: char array of KEYLENGTH+1 (+1 for '\0' character)
char const* getkey(struct BTNode* a);
//  key is hidden in "bulk", so use the following function to
//  read key string of length KEYLENGTH
//  it will make BULK_SIZE/2 searches on average
//  so try to use it as rarely as possible

// return value: 0 for failure (NULL a), 1 for success
int setkey(struct BTNode* a, char const kw[]);
//  the following function hides a string "kw" of KEYLENGTH
//  by randomly selecting the location to save key

// copies the key of one node to the other
int copykey(struct BTNode* dst, struct BTNode* src) { return setkey(dst, getkey(src)); }
// very simple, single line, so implementation is given here

// return value: (by character comparison)
//  -1 if a's key < b's key
//  0 if a's key == b's key
//  +1 if a's key > b's key
//  may be needed for binary search tree search and build-up
int comparekey(struct BTNode* a, struct BTNode* b);

// return value: pointer to a single BTNode (left/right are NULL)
struct BTNode* generate_btnode(char const kw[]);
//  generates a node for binary tree

// frees a binary tree
void free_bt_recursive(struct BTNode* bt);

// return value: pointer to the root of the copy of the given binary tree "bt"
struct BTNode* copy_bt_recursive(struct BTNode* bt);

//  adds a node to the left of a BTNode parent
//  it will be used to generate a left-half binary tree
//  (LHBT, all rights are NULL)
// pre-condition: left pointer to the new node should be NULL
// to store the left pointer to the parent node
// return value: parent if the given parent is not NULL; newPtr if parent NULL
struct BTNode* insert_left_bcnode(struct BTNode* parent, struct BTNode* newPtr);

// File I/O: read key words from the given file
// and generate a binary tree which is left-half
// (all right children are NULL)
struct BTNode* readkeys_textfile_LHBT(char const infile[], int* pN);

/////////////////////////////////////////////////////////////
// FILL 1: generate a binary search tree using insertion
/////////////////////////////////////////////////////////////
struct BTNode* insert_to_BST_leaf(struct BTNode* root, struct BTNode* newPtr) {
    if (!root) return newPtr;  // New root
    if (!newPtr) return root;  // Nothing to insert

    // NOTE: comparekey did not adhere to the usual C comparison function rules,
    //       so I had to switch to a normal function.

    if (strcmp(getkey(root), getkey(newPtr)) < 0) {
        // If the value under the new node is bigger than the value stored in the root -
        // store it in the right subtree
        root->right = insert_to_BST_leaf(root->right, newPtr);
    } else {
        // Otherwise (smaller or equal): store it in the left subtree
        root->left = insert_to_BST_leaf(root->left, newPtr);
    }

    return root;
}

struct BTNode* generate_BST_by_insertion(struct BTNode* lhbt) {
    struct BTNode* bst = NULL;
    while (lhbt) {
        // Pop root node from lhbt
        struct BTNode* popped = lhbt;
        lhbt = lhbt->left;
        popped->left = NULL;

        // Insert into bst
        bst = insert_to_BST_leaf(bst, popped);
    }
    return bst;
}

/////////////////////////////////////////////////////////////
// FILL 2: PRINT
/////////////////////////////////////////////////////////////
// prints left-half binary tree
// ___-___-___
// INPUT
//   fp: file pointer for the output file, stdout for monitor output
//   lhbt: left-half binary tree (right pointers are all null)
// RETURNs number of NODES in the list
int print_LHBT(FILE* fp, struct BTNode* lhbt) {
    int num_nodes = 0;

    while (lhbt) {
        // Ensure it's a left-half tree
        assert(!lhbt->right);

        num_nodes++;
        fprintf(fp, "%s", getkey(lhbt));
        if (lhbt->left) fprintf(fp, "-");

        lhbt = lhbt->left;
    }

    // Add a newline at the end
    fputc('\n', fp);

    return num_nodes;
}

// prints a binary search tree nodes by a single line
// in a SORTED ORDER
// (hint: inorder traversal)
// INPUT
//   fp: file pointer for the output file, stdout for monitor output
//   bst: root node of the BST, should satisfy the property of
//      binary search tree, left <= center < right
//   level: level of the root node, starting from 0 (empty)
//      if it is unnecessary, do not have to use it
// RETURNs number of NODES in the list
int print_BST_sortedorder(FILE* fp, struct BTNode* bst, int level) {
    if (!bst) return 0;
    int count = 1;

    count += print_BST_sortedorder(fp, bst->left, level + 1);
    fprintf(fp, "%s ", getkey(bst));
    count += print_BST_sortedorder(fp, bst->right, level + 1);

    if (level == 0) fputc('\n', fp);

    return count;
}

// prints a binary search tree, rotated by 270 degrees
// Note: key's length is fixed to KEYLENGTH, so there are
// (KEYLENGTH+1)*level spaces. For examples,
//         999
//     777
//         555
// 333
//     222
//         111
// INPUT
//   (same as print_BST_sortedorder)
// RETURNs HEIGHT-1 of the printed tree (2 in the above example)
//   (hint: printing order is right -> center -> left
//    carefully count the number of spaces)
int print_BST_right_center_left(FILE* fp, struct BTNode* bst, int level) {
    // TODO: Compress the tree (wtf)
    if (!bst) return 0;

    int height = 1;
    int subtree_height = 0;

    subtree_height = print_BST_right_center_left(fp, bst->right, level + 4) + 1;
    if (subtree_height > height) height = subtree_height;

    fprintf(fp, "%*s%s\n", level, "", getkey(bst));

    subtree_height = print_BST_right_center_left(fp, bst->left, level + 4) + 1;
    if (subtree_height > height) height = subtree_height;

    return height;
}

/////////////////////////////////////////////////////////////
// FILL 3: Conversion of an BST to a complete BST
/////////////////////////////////////////////////////////////
// Source: https://stackoverflow.com/a/26896494

int pop_bst_nodes_in_order_to_array(struct BTNode* bst, struct BTNode** array, int index) {
    if (!bst) return index;

    index = pop_bst_nodes_in_order_to_array(bst->left, array, index);
    array[index++] = bst;
    index = pop_bst_nodes_in_order_to_array(bst->right, array, index);

    return index;
}

// previous_power_of_2 finds the biggest power of 2 that is not greater than given number.
// Examples:
//     previous_power_of_2(0) = 0
//     previous_power_of_2(1) = 1
//     previous_power_of_2(2) = 2
//     previous_power_of_2(3) = 2
//     previous_power_of_2(4) = 4
//     previous_power_of_2(7) = 4
//     previous_power_of_2(9) = 8
static inline unsigned previous_power_of_2(unsigned x) {
    static_assert(sizeof(unsigned) == 4,
                  "previous_power_of_2 requires `unsigned int` to be 32 bits for offset "
                  "calculation with CLZ");
    return x <= 1u ? x : 1u << (31u - __builtin_clz(x));
}

static inline unsigned find_complete_tree_partition(int length) {
    int perfect_subtree_size = (int)previous_power_of_2((unsigned)length);

    if ((perfect_subtree_size >> 1) - 1 > length - perfect_subtree_size) {
        // Right subtree must be perfect with `perfect_subtree_size / 2` nodes
        return length - (perfect_subtree_size >> 1);
    } else {
        // Left subtree must be perfect with `perfect_subtree_size` nodes
        return perfect_subtree_size - 1;
    }
}

struct BTNode* build_complete_tree(struct BTNode** array, int left, int right) {
    if (left > right) return NULL;

    int middle = left + find_complete_tree_partition(right - left + 1);
    struct BTNode* root = array[middle];

    // Build both subtrees
    root->left = build_complete_tree(array, left, middle - 1);
    root->right = build_complete_tree(array, middle + 1, right);

    return root;
}

// convert a BST to complete BST (minimum height, filling in left first)
// INPUT
//   bst: root node of the BST, should satisfy the property of
//      binary search tree, left <= center < right
//   numNodes: number of nodes in the bst
//      if not necessary in your implementation, do not have to use it
// RETURNs a COMPLETE BST
// (hint: using extra memory (arrays or lists) may help,
//  array's rule for parent-child, sorted list, etc.)
struct BTNode* BST_to_completeBST(struct BTNode* bst, int numNodes) {
    // Create a VLA on the stack for storing ordered nodes
    // Deliberately using a VLA, as going through libc and possibly a syscall
    // for memory allocation is **slow**.
    struct BTNode* nodes[numNodes];

    // Pop elements into the nodes array, so that the array is sorted
    pop_bst_nodes_in_order_to_array(bst, nodes, 0);

    // Reconstruct the tree
    return build_complete_tree(nodes, 0, numNodes - 1);
}

/////////////////////////////////////////////////////////////
// main function
/////////////////////////////////////////////////////////////
#define MAXLINE 1024
int main(int argc, char* argv[]) {
    int numWords;  // number of words
    // int wordLen;	// word length: number of characters per word
    struct BTNode *root, *bst1;
    int numNodes, lev;  // level of the tree

    /* for file name, max length 1023 including path */
    char infile[MAXLINE], outfile[MAXLINE];
    FILE* fp;

#ifdef MEASURE_TIME
    clock_t start, end;
    double cpu_time_used;
#endif

    if (argc != 3) {
        fprintf(stderr, "usage: %s input output\n", argv[0]);
        exit(0);
    } else {
        strcpy(infile, argv[1]);
        strcpy(outfile, argv[2]);
    }

    /* read text file of integers:
     * number_of_intergers integer1 integer2 ...
     * then convert it to a linked list */
    root = readkeys_textfile_LHBT(infile, &numWords);

    /* open output file pointer */
    fp = fopen(outfile, "w");
    if (fp == NULL) {
        fprintf(stderr, "cannot open file '%s' for write\n", outfile);
        fprintf(stderr, "output to stdout\n");
        fp = stdout;
    }

#ifdef MEASURE_TIME
    start = clock();
#endif

    if (root != NULL) {
        // prints input
        fprintf(fp, "=====================================\n");
        numNodes = print_LHBT(fp, root);
        fprintf(fp, "total %d nodes\n", numNodes);

        // BST construction by simple insertion
        // keep root unchanged
        bst1 = generate_BST_by_insertion(copy_bt_recursive(root));

        fprintf(fp, "=====================================\n");
        numNodes = print_BST_sortedorder(fp, bst1, 0);
        fprintf(fp, "total %d nodes (sorted)\n", numNodes);
        fprintf(fp, "=====================================\n");
        lev = print_BST_right_center_left(fp, bst1, 0);
        fprintf(fp, "BST height %d\n", lev);
        fprintf(fp, "=====================================\n");
        bst1 = BST_to_completeBST(bst1, numNodes);
        lev = print_BST_right_center_left(fp, bst1, 0);
        fprintf(fp, "Complete BST height %d\n", lev);
        fprintf(fp, "=====================================\n");

        free_bt_recursive(root);
        free_bt_recursive(bst1);
    }

#ifdef MEASURE_TIME
    end = clock();
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    fprintf(fp, "TIME %.5f seconds\n", cpu_time_used);
#endif

    if (fp != NULL && fp != stdout) fclose(fp);
    return 0;
}

/////////////////////////////////////////////////////////////
// implementation: functions for binary tree node
/////////////////////////////////////////////////////////////

char const* getkey(struct BTNode* a) {
    int i;
    for (i = 0; i < BULK_SIZE - KEYLENGTH; i++) {
        if (a->bulk[i] != '\0') return a->bulk + i;
    }
    return NULL;  // not found
}

int setkey(struct BTNode* a, char const kw[]) {
    int pos;
    if (a != NULL) {
        // fill with 0
        memset(a->bulk, 0, sizeof(char) * BULK_SIZE);

        // find position randomly to store KEYLENGTH+1 characters
        pos = rand() % (BULK_SIZE - KEYLENGTH);
        if (kw != NULL) memcpy(a->bulk + pos, kw, sizeof(char) * KEYLENGTH);
        a->bulk[pos + KEYLENGTH] = '\0';  // to make it a C string

        // success
        return 1;
    } else
        return 0;
}

struct BTNode* generate_btnode(char const kw[]) {
    struct BTNode* tmp;

    tmp = (struct BTNode*)malloc(sizeof(struct BTNode));
    setkey(tmp, kw);

    // initial left and right children for the generated leaf node
    tmp->left = tmp->right = NULL;

    return tmp;
}

void free_bt_recursive(struct BTNode* bt) {
    if (bt != NULL) {
        free_bt_recursive(bt->left);
        free_bt_recursive(bt->right);
        free(bt);
    }
}

struct BTNode* copy_bt_recursive(struct BTNode* bt) {
    struct BTNode* dup;

    if (bt != NULL) {
        dup = (struct BTNode*)malloc(sizeof(struct BTNode));
        copykey(dup, bt);
        dup->left = copy_bt_recursive(bt->left);
        dup->right = copy_bt_recursive(bt->right);
    } else
        dup = NULL;
    return dup;
}

struct BTNode* insert_left_bcnode(struct BTNode* parent, struct BTNode* newPtr) {
    if (parent == NULL)
        return newPtr;  // no parent
    else if (newPtr == NULL)
        return parent;  // Nothing to add
    else if (newPtr->left != NULL) {
        fprintf(stderr, "cannot add a node with non-null left tree\n");
        return parent;
    } else {
        newPtr->left = parent->left;
        parent->left = newPtr;
        return newPtr;  // returning new node as a new parent
    }
}

// static: internal use only
static int _compare_n_char(char const a[], char const b[], int L) {
    int i;
    for (i = 0; i < L; i++) {
        if (a[i] < b[i])
            return -1;
        else if (a[i] > b[i])
            return 1;
        else
            continue;  // to next character
    }
    return 0;
}

int comparekey(struct BTNode* a, struct BTNode* b) {
    return _compare_n_char(getkey(a), getkey(b), KEYLENGTH);
}

/////////////////////////////////////////////////////////////
// File I/O
/////////////////////////////////////////////////////////////
struct BTNode* readkeys_textfile_LHBT(char const infile[], int* pN)
// read key words from the given file
// and generate a binary tree which is skewed to the left
// (all right children are NULL)
{
    struct BTNode *root, *cur, *tmp;
    char word[1024];
    FILE* fp;
    int i;

    // check for input file name
    if (infile == NULL) {
        fprintf(stderr, "NULL file name\n");
        return NULL;
    }

    // check for file existence
    fp = fopen(infile, "r");
    if (!fp) {
        fprintf(stderr, "cannot open file %s\n", infile);
        return NULL;
    }

    // check for number of keys
    if (fscanf(fp, "%d", pN) != 1 || *pN <= 0) {
        fprintf(stderr, "File %s: ", infile);
        fprintf(stderr, "number of keys cannot be read or or wrong\n");
        fclose(fp);
        return NULL;
    }

    /*
    // check for number of characters per key
    if ( fscanf(fp, "%d", pL) != 1 || *pL <= 0 ) {
      fprintf(stderr, "File %s: ",infile);
      fprintf(stderr, "number of characters per key cannot be read or or wrong\n");
      fclose(fp);
      return NULL;
    }
    */

    // reading keys
    root = cur = tmp = NULL;
    for (i = 0; i < (*pN); i++) {
        if (fscanf(fp, "%s", word) != 1) {
            fprintf(stderr, "cannot read a word at %d/%d\n", i + 1, (*pN));
            *pN = i;  // number of read keys so far
            break;
        } else {
            // check_and_correct_word(word, KEYLENGTH);

            // generate a new node
            tmp = generate_btnode(word);

            if (root == NULL)
                root = cur = tmp;
            else
                cur = insert_left_bcnode(cur, tmp);
        }
    }

    return root;
}
