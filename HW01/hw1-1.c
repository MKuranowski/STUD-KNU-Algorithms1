/* ID: 2020427681
* NAME: Mikolaj Kuranowski
* OS: MacOS 12.5.1
* Compiler version: clang 13.1.6
*/
// >>> (10/100) pts
// >>> IN THE TOP COMMENTS BLOCK
// >>> LINE 1: REPLACE WITH YOUR ID (IF YOU HAVE NON-NUMERIC, IGNORE IT)
// >>> Line 2: REPLACE WITH YOUR NAME (NO HANGUL)
// >>> DO NOT CHANGE OS AND Compiler, COMPILE AND RUN YOUR CODE ON THE LINUX MACHINE

#include<stdio.h>
#include<string.h>// for strlen and strcpy
#include<stdlib.h>// for malloc
struct student { int id; char name[128], major[128]; };

int main( void ) {
    struct student *myself;
    myself = (struct student*)malloc(sizeof(struct student));
    // >>> (50/100) pts
    // >>> IN THE FOLLOWING 3 LINES,
    // >>> REPLACE WITH YOUR ID, NAME, and MAJOR
    myself->id = 2020427681;
    strcpy(myself->name,"Mikolaj Kuranowski");
    strcpy(myself->major,"Computer Science & Engineering");
    printf("ID: %d\n", myself->id);
    printf("NAME: %s\n", myself->name);
    printf("MAJOR: %s\n", myself->major);
}
