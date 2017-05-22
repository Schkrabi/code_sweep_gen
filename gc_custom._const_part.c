/**
 * @name gc_custom.c
 * @author Mgr. Radomir Skrabal
 * This file contains implemetations of Custom Cheneys copying garbage collector for gc_custom.h
 */

#include "gc_custom.h"
#include "gc_cheney_base.h"
#include "garbage_collector.h"
#include <unistd.h>
#include <string.h>
#include "gc_util.h"
#include <syslog.h>
#include "gc_cheney_base.h"

/**
 * Initializes the Garbage Collector objects
 * @return If everything went well 0, otherwise error code
 */
int gc_custom_init()
{
    void *chunk;   

    chunk = get_memory_primitive(2*SEMISPACE_SIZE);
    if(chunk == NULL)
    {
        return 1;
    }
//     gc_cheney_base_to_space = init_block_from_chunk(chunk, (2*SEMISPACE_SIZE) - sizeof(block_t));
    gc_cheney_base_to_space = (block_t*)((uint64_t)chunk + SEMISPACE_SIZE);
    if(gc_cheney_base_to_space == NULL)
    {
        return 2;
    }
//     gc_cheney_base_from_space = gc_cheney_base_get_mem((void*)&gc_cheney_base_to_space, SEMISPACE_SIZE - sizeof(block_t));
    gc_cheney_base_from_space = chunk;
    gc_cheney_base_semispace_middle = gc_cheney_base_to_space;
    if(gc_cheney_base_from_space == NULL)
    {
        return 3;
    }
    gc_cheney_base_remaining_block = gc_cheney_base_from_space;
    gc_cheney_base_roots_count = 0;
    
    return 0;
}

/**
 * Cleans up the garbage collectors objext
 * @return If everything went well 0, otherwise error code
 */
int gc_custom_cleanup()
{
    void *ptr = gc_cheney_base_from_space > gc_cheney_base_to_space ? gc_cheney_base_to_space : gc_cheney_base_from_space;
    release_memory_primitive(ptr);
}

/**
 * Allocates memory for single (non-array) value
 * @par type type number
 * @return pointer to allocated memory or NULL
 */
void *gc_custom_malloc(int type)
{
    block_t *block;
    
    if(type_table[type].size <= sizeof(uint64_t))
    {
        block = gc_cheney_base_alloc_block_of_size(0);
    }
    else
    {
        block = gc_cheney_base_alloc_block_of_size(type_table[type].size - sizeof(uint64_t));
    }
    
    if(block == NULL)
    {
        return block;
    }
    
    block_set_is_array(block, 0);
    block_set_type(block, type);
    block_set_array_size(block, 0);
    
    return get_data_start(block);
}

/**
 * Allocates memory for an array of values
 * @par type type number
 * @return pointer to allocated memory or NULL
 */
void *gc_custom_malloc_array(int type, size_t size)
{
    block_t *block;
    
    block = gc_cheney_base_alloc_block_of_size(type_table[type].size * size);
    
    if(block == NULL)
    {
        return block;
    }
    
    block_set_is_array(block, 1);
    block_set_type(block, type);
    block_set_array_size(block, size);
    
    return get_data_start(block);
}

/**
 * Carries out the "sweep" part of the algorithm
 * @return 0 if everything went well, error code otherwise
 */
int gc_custom_collect()
{
   return gc_custom_collect_from_roots(gc_cheney_base_roots, gc_cheney_base_roots_count);
}

/**
 * Carries out the "sweep" part of the algorithm
 * @par roots Array of root elements for garbage collection
 * @par size size of a roots arraay
 * @return 0 if everything went well, error code otherwise
 */
int gc_custom_collect_from_roots(root_ptr roots[], size_t size)
{
    block_t *todo_ptr;
    int i;
    
    gc_cheney_base_remaining_to_space = gc_cheney_base_to_space;
    todo_ptr = gc_cheney_base_to_space;
    
    for(i = 0; i < size; i++)
    {
      roots[i].ptr = gc_custom_scan_ptr(roots[i].ptr, TYPE_PTR, roots[i].is_array);
    }                                                                   
    
    while(todo_ptr < gc_cheney_base_remaining_to_space)
    {
      gc_custom_walk_block(todo_ptr);
      todo_ptr = next_block(todo_ptr);
    }
    
    gc_cheney_base_swich_semispaces();
    return 0;
}

/**
 * Scans the (copied) block of memory and copies the references it points to to the to_space
 * @par block memory block to be scanned
 * @return if everything went well 0 otherwise error code
 */
int gc_custom_walk_block(block_t *block)
{
    if(block_is_array(block))
    {
        return gc_custom_walk_array(block);
    }
    switch(block_get_type(block))
    {
        case TYPE_UNDEFINED:
        case TYPE_INT:
        case TYPE_DOUBLE:
            return 0;
        default:
            return gc_custom_walk_struct(block);
    }
}

/**
 * Scans the block of type MEM_TYPE_STRUCT
 * @par block block of memory of type MEM_TYPE_STRUCT
 * @return 0 if everything went well, error code otherwise
 */
int gc_custom_walk_struct(block_t *block)
{
    gc_custom_scan_struct(get_data_start(block), block_get_type(block));
}

/**
 * Returns the remaining space in bytes that collector has available
 * @return space in bytes or -1 if collector is limmited only by system
 */
int64_t gc_custom_remaining_space()
{
    return gc_cheney_base_remaining_space();
}