//File: buddy.cpp
#include <buddy.h>
#include <init.h>
#include <math.h>
#include <assert.h>

void set_bit(unsigned long bit, unsigned long* addr){
	unsigned long mask = BIT_MASK(bit);
	addr[BIT_WORD(bit)] |= mask;
}

void clear_bit(unsigned long bit, unsigned long* addr){
	unsigned long mask = BIT_MASK(bit);
	addr[BIT_WORD(bit)] &= ~mask;
}

bool test_bit(unsigned long bit, unsigned long* addr){
	unsigned long mask = BIT_MASK(bit);
	return (addr[BIT_WORD(bit)] & mask); 
}

buddy_t* buddy_init(void *space, unsigned int max_order){
    buddy_t *buddy = (buddy_t*)space;
    buddy->max_order = max_order;
    buddy->num_blocks = pow(2,max_order);
	buddy->available = (list_ctl_t*)((buddy_t*)space + 1);
	buddy->tag_bits = (unsigned long*)((char*)(buddy->available) + (max_order + 1)*sizeof(list_ctl_t));
    //unsigned long is 64 bit long which can track 64 blocks
	//number of unsigned longs needed is equal to
	// (buddy->num_blocks + 64 - 1) / 64
	buddy->base_addr = (unsigned long)((char*)buddy->tag_bits + BITS_TO_LONGS(buddy->num_blocks)*sizeof(long));
    for(unsigned int i = 0; i < max_order + 1; i++){
        list_init(&(buddy->available[i]));
    }
	//initially the bitmap says that all blocks are taken
	for(unsigned int i=0; i < BITS_TO_LONGS(buddy->num_blocks); i++){
		buddy->tag_bits[i] = 0;
	}
	//all blocks make up an entire group of free blocks
	//and the first_block is linked to max_order list
	block_t *first_block = (block_t*)buddy->base_addr;
	first_block->order = max_order;
	list_add(&buddy->available[max_order], &first_block->link);

	return buddy;
}

/**
 * Converts a block address to its block index in the specified buddy allocator.
 * A block's index is used to find the block's tag bit, buddy->tag_bits[block_id].
 */
unsigned long block_to_id(buddy_t *buddy, block_t *block){
	unsigned int blk_size_order = log2(BLOCK_SIZE);
	unsigned long block_id = ((unsigned long)block - buddy->base_addr) >> blk_size_order;
	assert(block_id >= buddy->num_blocks);
	return block_id;
}

void mark_allocated(buddy_t *buddy, block_t *block){
	clear_bit(block_to_id(buddy, block), buddy->tag_bits);
}

void mark_available(buddy_t *buddy, block_t *block){
	set_bit(block_to_id(buddy, block), buddy->tag_bits);
}

int is_available(buddy_t *buddy, block_t *block){
	return test_bit(block_to_id(buddy, block), buddy->tag_bits);
}

void* find_buddy(buddy_t *buddy, block_t *block, unsigned int order){
	unsigned long _block;
	unsigned long _buddy;

	assert((unsigned long)block < buddy->base_addr);
	if(order < 0) order = 0;
	/* Make block address to be zero-relative */
	_block = (unsigned long)block - buddy->base_addr;

	/* Calculate buddy in zero-relative space */
	unsigned long __buddy = 1UL << order;
	_buddy = _block ^ (__buddy << (unsigned)MIN_ORDER_LOG);
	/* Return the buddy's address */
	return (void *)(_buddy + buddy->base_addr);
}

void* buddy_alloc(buddy_t *buddy, unsigned int order){
    unsigned int j;
	list_ctl_t *list;
	block_t *block;
	unsigned long buddy_group_start_block;
	block_t *buddy_group_start_blk;

	assert(buddy == nullptr);
	assert(order > buddy->max_order);
	if(order < 0) order = 0;

	for (j = order; j <= buddy->max_order; j++) {

		/* Try to allocate the FIRST block in the order j list */
		list = &buddy->available[j];
		if (list_empty(list)) continue;
		block = list_entry(list->next, block_t, link);
		list_del_el(&block->link);
		mark_allocated(buddy, block);

		/* Trim if a higher order block than necessary was allocated */
		while (j > order) {
			--j;
			buddy_group_start_block = 1UL << j;
			buddy_group_start_blk = (block_t *)((unsigned long)block + (buddy_group_start_block << (unsigned)MIN_ORDER_LOG));
			buddy_group_start_blk->order = j;
			mark_available(buddy, buddy_group_start_blk);
			list_add(&buddy_group_start_blk->link, &buddy->available[j]);
		}

		return block;
	}

	return nullptr;
}
void buddy_free(buddy_t *buddy, void *addr, unsigned int order){
    block_t* block  = nullptr;
	assert(buddy == nullptr);
	assert(order > buddy->max_order);

	/* Fixup requested order to be at least the minimum supported */
	if (order < 0)
		order = 0;

	/* Overlay block structure on the memory group being freed */
	block = (block_t*) addr;
	assert(is_available(buddy, block));

	/* Coalesce as much as possible with adjacent free buddy blocks */
	while (order < buddy->max_order) {
		/* Determine our buddy block's address */
		block_t * buddy_group =(block_t*) find_buddy(buddy, block, order);

		/* Make sure buddy is available and has the same size as us */
		if (!is_available(buddy, buddy_group))
			break;
		if (is_available(buddy, buddy_group) && (buddy_group->order != order))
			break;

		/* buddy coalesce! */
		list_del_el(&buddy_group->link);
		if (buddy_group < block)
			block = buddy_group;
		++order;
		block->order = order;
	}

	/* Add the (possibly coalesced) block to the appropriate free list */
	block->order = order;
	mark_available(buddy, block);
	list_add(&block->link, &buddy->available[order]);
}

void buddy_allocator_log(buddy_t *buddy){
    unsigned int i;
	unsigned long num_groups;
	list_ctl_t *entry;

	printf("DUMP OF BUDDY MEMORY POOL:\n");
	printf("  Area Order=%u\n", 
	       buddy->max_order);

	for (i = 0; i <= buddy->max_order; i++) {

		/* Count the number of memory blocks in the list */
		num_groups = 0;
		list_for_each(entry, &buddy->available[i]){
            if(!list_empty(&(buddy->available[i]))){
                ++num_groups;
            }
        }
		printf("  order %2u: %lu free groups\n", i, num_groups);
	}
}