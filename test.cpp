#include <iostream>
#include <buddy.h>
#include <stdlib.h>
#include <math.h>
using namespace std;
#define BLOCK_NUMBER 256

int main(int argc, char *argv[]){
    void* space = malloc(BLOCK_SIZE * BLOCK_NUMBER);
    buddy_t* buddy_allocator = buddy_init(space, floor(log2(BLOCK_NUMBER)));
    buddy_allocator_log(buddy_allocator);
    block_t* group1 = (block_t*)buddy_alloc(buddy_allocator, 0);
    buddy_allocator_log(buddy_allocator);
    block_t* group2 = (block_t*)buddy_alloc(buddy_allocator, 4);
    buddy_allocator_log(buddy_allocator);
    block_t* group3 = (block_t*)buddy_alloc(buddy_allocator, 6);
    buddy_allocator_log(buddy_allocator);
    block_t* group4 = (block_t*)buddy_alloc(buddy_allocator, 4);
    buddy_allocator_log(buddy_allocator);
    block_t* group5 = (block_t*)buddy_alloc(buddy_allocator, 6);
    buddy_allocator_log(buddy_allocator);
    buddy_free(buddy_allocator, (void*)group1, 0);
    buddy_allocator_log(buddy_allocator);

    free(space);
}
