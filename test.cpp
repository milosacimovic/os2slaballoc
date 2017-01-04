#include <iostream>
#include <buddy.h>
#include <stdlib.h>
#include <math.h>
using namespace std;
#define BLOCK_NUMBER 5

int main(int argc, char *argv[]){
    void* space = malloc(BLOCK_SIZE * BLOCK_NUMBER);
    buddy_t* buddy_allocator = buddy_init(space, floor(log2(BLOCK_NUMBER)));
    buddy_allocator_log(buddy_allocator);
    free(space);
}
