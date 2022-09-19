/* ID: 2020427681
 * NAME: Mikolaj Kuranowski
 * OS: MacOS 12.6
 * Compiler version: clang 14.0.0
 */

// >>> (10/100) pts
// >>> IN THE TOP 4-LINES COMMENTS
// >>> LINE 1: REPLACE WITH YOUR ID (IF YOU HAVE NON-NUMERIC, IGNORE IT)
// >>> Line 2: REPLACE WITH YOUR NAME (NO HANGUL)
// >>> DO NOT CHANGE OS AND Compiler
// >>> COMPILE AND RUN YOUR CODE ON THE LINUX MACHINE

// HOMEWORK PROGRAMMING ASSIGNMENT 2-1
// TIME MEASUREMENT USING FUNCTION clock() defined in time.h
// MEMORY USAGE MEASUREMENT USING malloc_c() AND strdup_c()
// WHICH REPLACE THE BUILT-IN FUNCTIONS malloc() and strdup()

#include<stdio.h>
#include<stdlib.h>
#include<string.h>	// string library
#include<time.h>	// time library

// TIME
// THE FOLLOWING FUNCTIONS SHOW HOW TO MEASURE THE EXECUTION TIME
// USING A BUILT-IN FUNCTION clock() DEFINED IN time.h
// NOTE: STATIC VARIABLES ARE NECESSARY TO RECORD CLOCKS
// USAGE:
//    reset_timer();	// reset the start time
//    ....		// statements to measure time
//    t = elapsed_time_in_sec();
//    		// time in seconds from when reset_timer() was called

static clock_t clocks_start;	// global static variable for start clock
static void reset_timer()
{
  clocks_start = clock();	// record the current clock ticks
}

static double elapsed_time_in_sec()
  // returns time in seconds from the start
{
  return ((double) (clock() - clocks_start)) / CLOCKS_PER_SEC;
}

// MEMORY
// Given (allowed): malloc_c(size_t) strdup_c(const char*)
// Allowed string functions: strcpy, strncpy, strlen, strcmp, strncmp
// Unallowed memory functions: memcpy, memccpy, memmove, wmemmove,
//    or other direct memory copy/move functions
//    these functions performs 'BLOCKED' operations so that
//    a large chunk of memory allocation or move operation are
//    efficiently implemented, so they break UNIT TIME assumption
//    in algorithm design
// Unallowed string functions: strdup

/////////////////////////////////////////////////////////////////////
// to compute used memory
// use malloc_c defined below, instead of malloc, calloc, realloc, etc.
// malloc_c accumulates the size of the dynamically allocated memory to
// global/static variable used_memory, so that we can measure the
// used amount of memory exactly.
/////////////////////////////////////////////////////////////////////
static size_t used_memory = 0;	// intial used memory is 0
static size_t used_memory_in_bytes()
  // returns the number of bytes allocated by malloc_c and strdup_c
{
  return used_memory;
}

void *malloc_c(size_t size)
{
  if ( size > 0 ) {
    // increase the required memory count
    used_memory += size;
    return malloc(size);
  }
  else return NULL;
}

// create a duplicate word with counting bytes
char *strdup_c(const char *s) {
  int size;
  size = strlen(s)+1;   // including last null character
  used_memory += size;
  return strdup(s);
}


// DO NOT USE malloc() and strdup()
// the below two lines detects unallowed usage of malloc and strdup
// NULL pointer will be returned, causing runtime errors
// NOTE: '#define' is effective only after declaration
#define malloc	NOT_ALLOWED
#define strdup	NOT_ALLOWED


/////////////////////////////////////////////////////////////
// start of main codes
/////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////
// print char array
/////////////////////////////////////////////////////////////
void print_chararr( FILE *fp, char *A[], int N )
  // A[]: char string array to print
  // N:   size of the array
  // fp:  file pointer, stdout or stderr for screen display
{
  int i;
  fprintf(fp,"%d\n",N);
  for (i=0; i<N; i++) fprintf(fp,"%s ",A[i]);
  fprintf(fp,"\n");
}

void free_chararr( char **A, int N )
  // A: char string array to free
  // N:   size of the array
{
  int i;
  for (i=0; i<N; i++) free(A[i]);
  free(A);
}

/////////////////////////////////////////////////////////////
// read words from a text file
// NOTE: using malloc_c() and strdup_c()
/////////////////////////////////////////////////////////////

// maximum length of a single string (word)
#define MAX_WORD_LEN	256

char **read_chararr_textfile( const char infile[], int *pN )
  // returns an array of words, with its size stored in
  // the memory indicated by integer pointer variable pN
  // the retured memory should freed by the caller
{
  int i;
  FILE *fp;
  char buf[MAX_WORD_LEN];	// temporary string for fscanf
  char **A;

  // NOTE: a lot of part of the code below are file I/O error checking
  // simple code (without error checking)
  /*
  fp = fopen(infile,"r");
  fscanf(fp, "%d", pN);
  A = (char**)malloc_c(sizeof(char*)*(*pN));
  for (i=0; i<(*pN); i++) {
    fscanf(fp, "%s", buf);
    A[i] = strdup_c(buf);
  }
  fclose(fp);
  return A;
  */

  // check for input file name
  if ( infile == NULL ) {
    fprintf(stderr, "NULL file name\n");
    return NULL;
  }

  // check for file existence
  fp = fopen(infile,"r");
  if ( !fp ) {
    fprintf(stderr, "cannot open file %s\n",infile);
    return NULL;
  }
  else {
    // check for number of elements
    if ( fscanf(fp, "%d", pN) != 1 || *pN <= 0 ) {
      fprintf(stderr, "cannot read number of elements %s\n",infile);
      return NULL;
    }
    else {
      A = (char**)malloc_c(sizeof(char*)*(*pN));
      for (i=0; i<(*pN); i++) {
	if ( fscanf(fp, "%s", buf) != 1 ) {
	  fprintf(stderr, "cannot read value at %d/%d\n",i+1,(*pN));
	  *pN = i;	// read data items
	  fclose(fp);
	  return A;
	}
	else {
	  A[i] = strdup_c(buf);	// copy the word stored in buf
	}
      }
      fclose(fp);
      return A;
    }
  }
}

/////////////////////////////////////////////////////////////
// write words to a text file
/////////////////////////////////////////////////////////////
void write_chararr_textfile( const char outfile[],
    char *A[], int N )
  // write the given array of int string words, with its sie N
  // to file whose name given by outfile[]
  // uses print_chararr
{
  FILE *fp;

  // NOTE: a lot of part of the code below are file I/O error checking
  // simple code (without error checking)
  /*
  fp = fopen(outfile,"w");
  print_chararr(fp,A,N);
  fclose(fp);
  */

  // check for output filename
  if ( outfile == NULL ) {
    fprintf(stderr, "NULL file name\n");
    return;
  }

  // check for file existence
  fp = fopen(outfile,"w");
  if ( !fp ) {
    fprintf(stderr, "cannot open file for write %s\n",outfile);
  }
  else {
    print_chararr(fp,A,N);
    fclose(fp);
  }
}

/////////////////////////////////////////////////////////////
// bubble sort
// source: https://ko.wikipedia.org/
/////////////////////////////////////////////////////////////
void bubble_sort_chararr(char **arr, int n) {
  // moving array pointers
  int i, j;
  char *temp;
  for (i=n-1; i>0; i--) {
    for (j=0; j<i; j++) {
      if (strcmp(arr[j], arr[j+1]) > 0 ) {
	temp = arr[j];
	arr[j] = arr[j+1];
	arr[j+1] = temp;
      }
    }
  }
}

/* NOTE: addtional funcstions can be defined */

#ifdef __GNUC__
#  define ALWAYS_INLINE __attribute__((always_inline))
#else
#  define ALWAYS_INLINE
#endif

ALWAYS_INLINE static inline void swap_char_pointer(char** restrict a, char** restrict b) {
    char* tmp = *a;
    *a = *b;
    *b = tmp;
}

/////////////////////////////////////////////////////////////
// insertion sort
// source: https://en.wikipedia.org/wiki/Insertion_sort#Algorithm
/////////////////////////////////////////////////////////////
void insertion_sort_chararr(char **arr, int n) {
    for (int i = 1; i < n; ++i) {
        for (int j = i; j > 0 && strcmp(arr[j - 1], arr[j]) > 0; --j) {
            swap_char_pointer(arr + j - 1, arr + j);
        }
    }
}

/////////////////////////////////////////////////////////////
// selection sort
// source: https://en.wikipedia.org/wiki/Selection_sort#Implementations
/////////////////////////////////////////////////////////////
void selection_sort_chararr(char **arr, int n) {
    for (int i = 0; i < n - 1; ++i) {
        int j_min = i;
        for (int j = i + 1; j < n; ++j) {
            if (strcmp(arr[j], arr[j_min]) < 0) {
                j_min = j;
            }
        }

        if (j_min != i) swap_char_pointer(arr + j_min, arr + i);
    }
}

/////////////////////////////////////////////////////////////
// main function
/////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
  int n, num_words;
  int method;
  char **A;	// to store data to be sorted
  char **B;	// to store re-ordered strings

  if ( argc != 5 ) {
    fprintf(stderr, "argc = %d\n",argc);
    fprintf(stderr, "usage: %s method infile sortedfile revsortedfile\n",
	argv[0]);
    fprintf(stderr, " method = 1 --- bubble sort\n"
	" method = 2 --- insertion sort\n"
	" method = 3 --- selection sort\n");
    exit(0);
  }

  method = atoi(argv[1]);

  /* read text file of words:
   * number_of_intergers word1 word2 ... */
  A = read_chararr_textfile(argv[2], &num_words);

  // start timer
  reset_timer();

  // sort the string array A
  switch ( method ) {
    case 1: bubble_sort_chararr(A, num_words);
	    break;
    case 2: insertion_sort_chararr(A, num_words);
	    break;
    case 3: selection_sort_chararr(A, num_words);
	    break;
  }

  // reverse the order of words in A and store it to B
  B = (char**) malloc_c(sizeof(char*)*num_words);
  for (n=0; n<num_words; n++) B[num_words-n-1] = strdup_c(A[n]);

  // display computation time and memory usage
  // NOTE: file I/O time not included
  fprintf(stdout,"TIME: %.5f seconds\n", elapsed_time_in_sec());
  fprintf(stdout,"MEMORY USAGE: %ld bytes\n", used_memory_in_bytes());

  // save results
  write_chararr_textfile(argv[3], A, num_words);
  write_chararr_textfile(argv[4], B, num_words);

  // free A and B
  free_chararr(A, num_words);
  free_chararr(B, num_words);
}
