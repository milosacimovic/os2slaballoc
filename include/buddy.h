//File: buddy.h
#ifndef _BUDDY_H_
#define _BUDDY_H_

#include <list.h>
#define BITS_IN_BYTE 8
#define BITS_PER_LONG (sizeof(long) * BITS_IN_BYTE)
#define MIN_ORDER_LOG (log2(BLOCK_SIZE))
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#define BITS_TO_LONGS(num) DIV_ROUND_UP(num, BITS_PER_BYTE * sizeof(long))
#define BIT_MASK(num) (1UL << ((num) % BITS_PER_LONG))
#define BIT_WORD(num) ((num) / BITS_PER_LONG)

typedef struct buddy_s{
  unsigned long base_addr;    /** base address of the memory pool */
	unsigned int    max_order;   /** 2^maxorder blocks in total */
	unsigned long    *tag_bits; /** one bit for each block
	                                *   0 = block is allocated
	                                *   1 = block is available
	                                */
  unsigned int num_blocks;
  list_ctl_t *available; //list of available groups
} buddy_t;

typedef struct free_group{
  list_ctl_t link;
  unsigned int order;
} block_t;


void buddy_init(void *space, int block_num);
/**
* This function initializes the beginning of the memory
* with a structure that controles the buddy block allocation
* and deallocation
* @space: pointer to first memory location
* @block_num: number of blocks that need to be managed
*/
void* buddy_alloc(buddy_ctl_t *buddy, unsigned int order);
/**
* This function allocates 2^order blocks and
* returns the address if the allocation was successful
* @order: 2^order(blocks) sized group is allocated
*/
void buddy_free(buddy_ctl_t *buddy, void *addr, unsigned int order);
/**
* This function frees the group of blocks
* @addr: address of the first block in the group
* @order: 2^order(blocks) sized group
*/
void buddy_allocator_log(buddy_ctl_t *buddy);
/**
* This function initializes logs the current
* state of the buddy allocator
*/

#endif
