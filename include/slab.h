// File: slab.h
#ifndef _SLAB_H_
#define _SLAB_H_
#include <list.h>
#include <stdio.h>
#include <mutex>
#define CACHE_NAME 20

typedef struct kmem_cache_s{
    char name[CACHE_NAME]; /*name of this cache*/
    size_t obj_size; /*size of objects in this cache*/
    list_ctl_t slabs_full; /*list of full slabs*/
    list_ctl_t slabs_partial; /*list of partially full slabs*/
    list_ctl_t slabs_free; /*list of free slabs*/
    unsigned int total_num_obj; /*total number of objects in cache
                                --when the cache is grown
                                this gets incremented by num_obj_per_slab*/
    unsigned int num_obj_per_slab;    /* # of objs per slab 
                                        --this is calculated upon
                                        cache creation based off of
                                        size of the object BLOCK_SIZE and control
                                        structures on the slab (block_t and slab_t)*/
    unsigned int blocks_order_per_slab; /* each slab has 2^blocks_order_per_slab of blocks*/
    size_t color_range;         /* cache colouring range
                                (number of different allignments) */
    unsigned int color_next;    /* next colour to use 
                                --upon cache creation
                                this is set to 0
                                --when the cache is grown
                                it is saved in the created slab (slab->color)
                                and after is set to the next offset*/
    
    
    /*struct kmem_cache_s *slabp_cache; //pointer to the kernel cache that is used for 
                            //the slab_t, if the slab_t is maintained off-slab.*/
    void (*ctor)(void *);
    void (*dtor)(void *);
    list_ctl_t next;
    unsigned long failures; /*current failure code*/
    unsigned int growing;
    unsigned int shrinked;
    std::mutex* mutexp; /*a cache can be accessed by one thread at a time*/
    std::mutex mutex;
} kmem_cache_t;

#define BLOCK_SIZE (4096)
#define CACHE_L1_LINE_SIZE (64)

void kmem_init(void *space, int block_num);

kmem_cache_t *kmem_cache_create(const char *name, size_t size, void (*ctor)(void *), void (*dtor)(void *)); // Allocate cache

int kmem_cache_shrink(kmem_cache_t *cachep); // Shrink cache

void *kmem_cache_alloc(kmem_cache_t *cachep); // Allocate one object from cache

void kmem_cache_free(kmem_cache_t *cachep, void *objp); // Deallocate one object from cache

void *kmalloc(size_t size); // Alloacate one small memory buffer

void kfree(const void *objp); // Deallocate one small memory buffer

void kmem_cache_destroy(kmem_cache_t *cachep); // Deallocate cache

void kmem_cache_info(kmem_cache_t *cachep); // Print cache info

int kmem_cache_error(kmem_cache_t *cachep); // Print error message

#endif
