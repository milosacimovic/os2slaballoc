//File: slab.cur
#include <slab.h>
#include <stdio.h>
#include <iostream>
#include <assert.h>
#include <buddy.h>
#include <string.h>
#include <math.h>
/**/
#define SIZES 13
/**/
#define GET_GROUP_CACHE(blk) ((kmem_cache_t*)(((block_t*)(blk))->link.next))
#define GET_GROUP_SLAB(blk) ((slab_t*)((block_t*)(blk) + 1))
#define SET_GROUP_CACHE(blk, cache) ((blk)->link.next = (list_ctl_t*)cache)
#define ADDR_TO_BLOCK(addr) ((unsigned long)addr)&(~(BLOCK_SIZE-1))//this is questionable
/**/
#define SLAB_BUFFER(slabp) ((buf_t*)(((slab_t*)(slabp))+1))
#define EOB -1
/**/
#define GET_SLAB_BLOCK(slabp) (((block_t*)(slabp))-1)
/*free buffer element*/
typedef unsigned int buf_t;
/*base address*/
unsigned long base_address;
/*buddy allocator*/
buddy_t* buddy_allocator;
/**/
std::mutex chain_mtx;
/*element of array of size-N cache control structure*/
typedef struct sizes_cs{
        size_t obj_size;
        kmem_cache_t* cachep;
}sizes_cs_t;


/*slab control structure*/
typedef struct slab_s {
        list_ctl_t list;
        unsigned long color; /*offset in this particular slab*/
        void *start_mem;  /* start_mem is the address of the first 
                          object(coloring included)*/
        unsigned int obj_inuse;  /* num of objs active in slab */
        buf_t free;
} slab_t;

void kmem_mutex_ctor(void* addr){
        kmem_cache_t *cachep = (kmem_cache_t*)addr;
        cachep->mutexp = new((void*)(&cachep->mutex)) std::mutex();
}
void kmem_mutex_dtor(void* addr){
        kmem_cache_t *cachep = (kmem_cache_t*) addr;
        cachep->mutexp->std::mutex::~mutex();
}

block_t* alloc_blocks(kmem_cache_t *cachep){
     block_t *addr;
     addr = buddy_alloc(buddy_allocator, cachep->blocks_order_per_slab);
     //ERROR: a group of blocks of sufficient size wasn't found
     if(addr == nullptr){
             cachep->failures = 1;
     }
     return addr;
}

void free_blocks(block_t* blocks){
        buddy_free(buddy_allocator, (void*)blocks, blocks->order);
}

void calculate_cache_params(kmem_cache_t* cachep){
        size_t obj_size = cachep->obj_size;
        unsigned int unused = 0;
        unsigned int order = 0;
        unsigned int available = BLOCK_SIZE - (sizeof(block_t) + sizeof(slab_t)+sizeof(buf_t));

        unsigned int obj_per_slab = 1;
        if(obj_size < available){
                while(obj_per_slab*obj_size + obj_per_slab*sizeof(buf_t) < available){
                        obj_per_slab++;
                }
                obj_per_slab--;
        }else{
                while(obj_size > available){ /*if the obj_size is greater than BLOCK_SIZE*/
                        order++;
                        available += BLOCK_SIZE;
                }
        }
        //ERROR: group of blocks is too big to handle
        if(order > buddy_allocator->max_order)
                cachep->failures = 2;
        unused = available - (obj_per_slab*obj_size + obj_per_slab*sizeof(buf_t));
       
        cachep->num_obj_per_slab = obj_per_slab;
        cachep->color_range = unused/CACHE_L1_LINE_SIZE;
        cachep->color_next = 0;
        cachep->blocks_order_per_slab = order;
}

void kmem_init(void *space, int block_num){
        
        /*Initialize the start of the space with kmem_cache_t
        structure that manages the cache of kmem_cache_t structures*/
        
        kmem_cache_t *cache_kmem = (kmem_cache_t*)space;
        base_address = (unsigned long)space;
        list_init(&cache_kmem->slabs_full);
        list_init(&cache_kmem->slabs_partial);
        list_init(&cache_kmem->slabs_free);
        
        cache_kmem->obj_size = sizeof(kmem_cache_t);
        
        strcpy(cache_kmem->name, "cache_kmem");
        
        /*cache chain resides in cache_kmem->next*/
        list_init(&cache_kmem->next);
        /*rest of cache_kmem initializiation*/
        cache_kmem->total_num_obj = 0;
        cache_kmem->ctor = kmem_mutex_ctor;
        cache_kmem->dtor = kmem_mutex_dtor;
        cache_kmem->failures = 0;
        cache_kmem->growing = 0;
        cache_kmem->shrinked = 0;
        
        cache_kmem->mutexp =new((void*)(&cache_kmem->mutex)) std::mutex();

        /*2^5, 2^6, 2^7, 2^8, 2^9, 2^10, 2^11, 2^12, 2^13, 2^14, 2^15, 2^16, 2^17,*/
        /*13 element array of sizes and pointers kmem_cache_t* for size-N caches*/
        size_t sizes = 32;
        sizes_cs_t* sizes_cs = (sizes_cs_t*)((kmem_cache_t*)space + 1);
        for(int i=0 ; i<SIZES; i++, sizes<<=1){
                sizes_cs[i].obj_size = sizes;
                sizes_cs[i].cachep = nullptr;}
        sizes_cs[SIZES].obj_size = 0;
        sizes_cs[SIZES].cachep = nullptr;
        /*creating buddy allocator interface*/
        sizes_cs += SIZES + 1;
        unsigned int max_order = floor(log2(block_num));
        
        buddy_allocator = buddy_init((void*)sizes_cs, max_order);
        /*calculating the color_range and slab size*/
        calculate_cache_params(cache_kmem);
}


/* function that creates a cache
 * checks for bad usage
 * allocates kmem_cache_t from cache_kmem
 * if a cache with the same name already exists
 * then the existing kmem_cache_t is returned
 *
 * @name: pointer to character array that is the name of cache
 * @size: size of an object which resides in cache
 * @ctor: constructor
 * @dtor: destructor
 */
kmem_cache_t *kmem_cache_create(const char *name, size_t size, void (*ctor)(void *), void (*dtor)(void *)){ // Allocate cache
        /*check for bad usage*/
        assert(name != nullptr);
        assert(strlen(name) < CACHE_NAME);
        //assert(size > BYTES_PER_WORD);
        if(dtor)
          assert(ctor);
        kmem_cache_t* cache_kmem = (kmem_cache_t*)base_address;
        kmem_cache_t* cache = (kmem_cache_t*)kmem_cache_alloc(cache_kmem);
        //ERROR: cache couldn't be allocated from cache_kmem
        if(!cache){
                return cache;
        }
        chain_mtx.lock();
                list_ctl_t *head = &cache_kmem->next;
                list_ctl_t *pos;
                list_for_each(pos, head){
                        kmem_cache_t* entry = list_entry(pos, kmem_cache_t,next);
                        if(strcmp(name, entry->name) == 0){
                                kmem_cache_free(cache_kmem, cache);
                                cache = entry;
                                return cache;
                        }
                }

                list_add(&cache_kmem->next, &cache->next);
        chain_mtx.unlock();
        strcpy(cache->name, name);
        cache->obj_size = size;
        list_init(&cache->slabs_full);
        list_init(&cache->slabs_partial);
        list_init(&cache->slabs_free);
        calculate_cache_params(cache);
        /*rest of kmem_cache_t initialization*/
        kmem_mutex_ctor(cache);
        cache->ctor = ctor;
        cache->dtor = dtor;
        cache->failures = 0;
        cache->growing = 0;
        cache->shrinked = 0;
        return cache;
        
}

/*Initialization of objects in a slab
 *@slab: slab whose objects are initialized
 */
void init_objs(kmem_cache_t* cachep, slab_t* slabp){
        unsigned int i;
        void* objp = slabp->start_mem;
        for(i = 1; i < cachep->num_obj_per_slab; i++){
                if(cachep->ctor != nullptr)
                        cachep->ctor(objp);
                objp = (void*)(((char*)objp)+cachep->obj_size);
        }
        for(i = 0;i < cachep->num_obj_per_slab - 1; i++){
                 SLAB_BUFFER(slabp)[i] = i+1;
        }
        SLAB_BUFFER(slabp)[cachep->num_obj_per_slab - 1] = EOB;
        slabp->free = 0;
}
/*function to create a new slab
 *@cachep: pointer to control structure 
 * of the cache is grown
 */
slab_t* add_slab(kmem_cache_t* cachep){
        slab_t* slab;
        size_t color;
        block_t* group;
        group = buddy_alloc(buddy_allocator, cachep->blocks_order_per_slab);
         //ERROR: buddy couldn't allocate anymore
         if(!group){
              return (slab_t*)group;
         }
        /*set up the color in this slab
        based on color_next and color_range*/
        color = cachep->color_next;
        cachep->color_next++;
        if (cachep->color_next >= cachep->color_range)
                 cachep->color_next = 0;
         color *= CACHE_L1_LINE_SIZE;
         /*keep track of to which slab and cache object belongs*/
         SET_GROUP_CACHE(group, cachep);
         slab = (slab_t*)(group + 1);
         /*total number of objects in a cache
          is increased by the number
         of objects in a slab*/
         cachep->total_num_obj += cachep->num_obj_per_slab;
         slab->color = color;
         slab->obj_inuse = 0;
         slab->start_mem = (void *)((unsigned char*)(((buf_t*)(slab + 1))+cachep->num_obj_per_slab)+slab->color);
         if(cachep->shrinked)
                cachep->growing = 1;
         /*initialize all objects and array 
         that keeps track of free objects*/
         init_objs(cachep, slab);
         /*add slab to slabs_free in cache*/
         list_add(&cachep->slabs_free, &slab->list);
         return slab;
}
/* function to return the address of an object from a given slab_s
 * @cachep: pointer to cache from which the object is allocated
 * @slabp: pointer to slab from the cache from which the object is allocated
 */
void* get_obj_from_slab(kmem_cache_t* cachep, slab_t* slabp){
        //if this was the last object allocated than this slab goes
        //from slabs_partial to slabs_full
        buf_t obj_ind = slabp->free;
        slabp->free = SLAB_BUFFER(slabp)[slabp->free];
        slabp->obj_inuse++;
        if(slabp->free == EOB){
                list_del_el(&slabp->list);
                list_add(&cachep->slabs_full, &slabp->list);
        }

        return ((void*)((char*)slabp->start_mem + obj_ind*cachep->obj_size));
}


/* function chooses the slab to allocate from
 * caller must garantee synchronization
 * @cachep: pointer to cache control structure
 * returns the slab from which the caller
 * allocates new object using function get_obj_from_slab
 */
slab_t* choose_slab(kmem_cache_t* cachep){
        list_ctl_t *slabs_partial, *first;
        slab_t* slab;
        slabs_partial = &cachep->slabs_partial;
        first = slabs_partial->next;
        if(list_empty(slabs_partial)){
                list_ctl_t *slabs_free = &cachep->slabs_free;
                first = slabs_free->next;
                if(list_empty(slabs_free)){
                        slab = add_slab(cachep);
                        if(!slab){
                                return slab;
                        }
                        first = &slab->list;
                }
                list_del_el(first);
                list_add(slabs_partial, first);
        }
        slab = list_entry(first, slab_t, list);
        return slab;
}
/*function calls the algorithm to choose a slab
 * and then allocates an object from it
 * @cachep: pointer to cache
 */
void* kmem_cache_alloc_helper(kmem_cache_t* cachep){
        void* objp;
        slab_t* slab;
        cachep->mutexp->lock();
        slab = choose_slab(cachep);
        if(!slab){
                cachep->mutexp->unlock();
                return slab;
        }
        objp = get_obj_from_slab(cachep, slab);
        cachep->mutexp->unlock();
        return objp;
}

/*function to allocate an object from the given cache
 * @cachep: pointer to given cache control structure
 */
void *kmem_cache_alloc(kmem_cache_t *cachep){
        return kmem_cache_alloc_helper(cachep);
}

/* function to return an object to a slab in a given cache
 * @cachep: pointer to cache from which the object is deallocated
 * @slabp: pointer to slab from the cache from which the object is deallocated
 * @objp: pointer to object being freed
 */
void ret_obj_to_slab(kmem_cache_t* cachep, slab_t* slabp,const void *objp){
        buf_t obj_ind = ((char*)objp - (char*)(slabp->start_mem))/cachep->obj_size;
        SLAB_BUFFER(slabp)[obj_ind] = slabp->free;
        slabp->free = obj_ind;
        unsigned int inuse = slabp->obj_inuse;
        slabp->obj_inuse--;
        if(slabp->obj_inuse == 0){
                list_del_el(&slabp->list);
                list_add(&cachep->slabs_free, &slabp->list);
        }else if(inuse == cachep->num_obj_per_slab){
                list_del_el(&slabp->list);
                list_add(&cachep->slabs_partial, &slabp->list);
        }
}
void ret_obj_to_slab(kmem_cache_t* cachep, slab_t* slabp, void *objp){
        if(cachep->dtor != nullptr){
                cachep->dtor(objp);
                if(cachep->ctor != nullptr) //should always happen
                        cachep->ctor(objp);
        }else{
                if(cachep->ctor != nullptr)
                        cachep->ctor(objp);
        }
        buf_t obj_ind = ((char*)objp - (char*)slabp->start_mem)/cachep->obj_size;
        SLAB_BUFFER(slabp)[obj_ind] = slabp->free;
        slabp->free = obj_ind;
        unsigned int inuse = slabp->obj_inuse;
        slabp->obj_inuse--;
        if(slabp->obj_inuse == 0){
                list_del_el(&slabp->list);
                list_add(&cachep->slabs_free, &slabp->list);
        }else if(inuse == cachep->num_obj_per_slab){
                list_del_el(&slabp->list);
                list_add(&cachep->slabs_partial, &slabp->list);
        }
}
/*function to find a slab for a given object and
 * deallocate from it that object
 * @cachep: pointer to cachep
 * @objp: pointer to object being deallocated
 */
void kmem_cache_free_helper(kmem_cache_t *cachep, const void *objp){
        slab_t* slab = GET_GROUP_SLAB(ADDR_TO_BLOCK(objp));//questionable
        cachep->mutexp->lock();
        ret_obj_to_slab(cachep, slab, objp);
        cachep->mutexp->unlock();
        
}
void kmem_cache_free_helper(kmem_cache_t *cachep, void *objp){
        slab_t* slab = GET_GROUP_SLAB(ADDR_TO_BLOCK(objp));//questionable
        cachep->mutexp->lock();
        ret_obj_to_slab(cachep, slab, objp);
        cachep->mutexp->unlock();
        
}
/*wrapper function around the function that determines
 * to what slab the object belongs
 * @cachep: pointer to cache
 * @objp: pointer to object
 */
void kmem_cache_free(kmem_cache_t *cachep, void *objp){
        assert(cachep != nullptr);
        if(!objp)
          return;
        kmem_cache_free_helper(cachep, objp);
}
/*function to call destructor for every object on the slab
 * and to give back the blocks allocted to the buddy memory pool
 * @cachep: pointer to cache
 * @slabp: pointer to slab
 */
void slab_destroy(kmem_cache_t* cachep, slab_t* slabp){
        if(cachep->dtor != nullptr){
                unsigned int i = 0;
                void *addr = (void*)(slabp->start_mem);
                for(; i < cachep->num_obj_per_slab; i++){
                        cachep->dtor(addr);
                        addr = (void*)((char*)addr+cachep->obj_size);
                }
        }
        block_t* group = GET_SLAB_BLOCK(slabp);
        free_blocks(group);
}
/* shrinkage of the cache by unlinking
 * any free slabs from the cache
 * @cachep: pointer to cache management
 * 
 */
int shrink_cache(kmem_cache_t* cachep){
        int ret = 0;
        while(!cachep->growing){
                if(list_empty(&cachep->slabs_free)) break;
                ret++;
                list_ctl_t* entry = cachep->slabs_free.next;
                slab_t* slab = list_entry(entry, slab_t, list);
                list_del_el(entry);
        }
        return ret;
}
/* wrapper function around shrink_cache function
 * mutually excludes anyone trying to access the cache
 * @cachep: pointer to cache
 */
int kmem_cache_shrink(kmem_cache_t *cachep){ //Deallocate slabs_free under the condition that
        int ret;
        cachep->mutex.lock();
        cachep->shrinked = 1;
        ret = shrink_cache(cachep);
        cachep->mutex.unlock();
        return ret << cachep->blocks_order_per_slab;
}

/* function to destroy a cache
 * unlinks given cache from cache chain 
 * deallocates kmem_cache_t for this cache
 * detects an error if not all lists were freed
 * @cachep: pointer to cache management
 *
 */
void kmem_cache_destroy(kmem_cache_t *cachep){ // Deallocate cache
        if(cachep == nullptr) return;
        chain_mtx.lock();
        list_del_el(&cachep->next);
        chain_mtx.unlock();
        cachep->growing = 0;
        shrink_cache(cachep);
        //ERROR: user hasn't freed all objects
        if(!list_empty(&cachep->slabs_full) || !list_empty(&cachep->slabs_partial))
              cachep->failures = 5;
        kmem_mutex_dtor((void*)cachep);
        kmem_cache_t* cache_kmem = (kmem_cache_t*)base_address;
        kmem_cache_free(cache_kmem, cachep);
}

/*function to allocate one object from size-N caches
 * creates a cache if it didn't exist
 *@size: size of the object
 */
void *kmalloc(size_t size){
     sizes_cs_t *sizes_cp = (sizes_cs_t*)((kmem_cache_t*)base_address + 1);
     
     for (; sizes_cp->obj_size; sizes_cp++) { //find the appropriate size
         if (size > sizes_cp->obj_size)
                continue;
         else{
                 
                if(!sizes_cp->cachep){
                        char name[20];
                        snprintf(name, sizeof(name), "size-%Zd", sizes_cp->obj_size);
                        sizes_cp->cachep = kmem_cache_create(name, sizes_cp->obj_size, nullptr, nullptr);
                }
                return kmem_cache_alloc_helper(sizes_cp->cachep);
         } 
     }
     //ERROR: user requested memory greater than available
     //cachep->failures = 4;
     return nullptr;
}
/* function to free object allocated with kmalloc
 * @objp: pointer to object being freed
 */
void kfree(const void *objp){
     kmem_cache_t *cache;
 
     if (!objp)
        return;
     cache = GET_GROUP_CACHE(ADDR_TO_BLOCK(objp));
     kmem_cache_free_helper(cache, objp);
}

/* log the current state of given cache function
 * @cachep: pointer to cache management
 */
void kmem_cache_info(kmem_cache_t *cachep){ // Print cache info
        if(cachep == nullptr) return;
        std::cout << "cache_name         object_size(bytes) cache_size(blk_no) slab_cnt           obj_cnt_per_slab   percentage(%)"<< std::endl;
        size_t slab_cnt = 0;
        size_t num_active_obj = 0;
        list_ctl_t *pos;
        unsigned int total_num;
        cachep->mutex.lock();
        total_num = cachep->total_num_obj;
        if(!list_empty(&cachep->slabs_full)){
                list_for_each(pos, &cachep->slabs_full){
                        slab_cnt++;
                        num_active_obj += cachep->num_obj_per_slab;
                }
        }
        if(!list_empty(&cachep->slabs_partial)){
                list_for_each(pos, &cachep->slabs_partial){
                        slab_cnt++;
                        slab_t* slab = (slab_t*)list_entry(pos, slab_t, list);
                        num_active_obj += slab->obj_inuse;
                }
        }
        if(!list_empty(&cachep->slabs_free)){
                list_for_each(pos, &cachep->slabs_partial){
                        slab_cnt++;
                }
        }
        cachep->mutex.unlock();
        printf("%-19s%-19Zd%-19Zd%-19Zd%-19u%-19.2f\n",cachep->name,cachep->obj_size, 
                (size_t)(pow(2, cachep->blocks_order_per_slab)*slab_cnt),
                slab_cnt, cachep->num_obj_per_slab, 
                num_active_obj ? (float)num_active_obj/total_num*100 : 0);
        

}
/* function to print an error message
 * @error_msg: pointer to character 
 * array that representes an error_msg
 */
void print_slab_error(){
        std::cout << "SLAB ALLOCATOR ERROR: ";
}
/* function to print an error message concerning
 * previous call of a function from the slab interface
 * @cachep: pointer to cache management
 */
int kmem_cache_error(kmem_cache_t *cachep){ // Print error message
        if(cachep == nullptr){
                print_slab_error();
                std::cout <<"kmem_cache_t couldn't be allocated from cache_kmem\n" << std::endl;
                return 3;
        }
        unsigned int failures = cachep->failures;
        switch(failures){
                case 1:
                        print_slab_error();
                        std::cout << "BUDDY ERROR: a group of blocks of sufficient size wasn't found.\n" << std::endl;
                        break;
                case 2:
                        print_slab_error();
                        std::cout << "BUDDY ERROR: group of blocks is too big to handle. Object too big for buddy.\n" << std::endl;
                        break;
                case 4:
                        print_slab_error();
                        std::cout <<" kmalloc: user requested memory greater\n than any of size-N caches can handle.\n" << std::endl;
                        break;
                case 5:
                        print_slab_error();
                        std::cout << "Can't destroy cache.\nUser hasn't freed all objects.\n" << std::endl;
                default :
                        break;
        }
        return failures;
}