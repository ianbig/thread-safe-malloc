#ifndef MY_MALLOC_H
#define MY_MALLOC_H

#include <pthread.h>
#include <stddef.h>
// meta information
enum MEM_TYPE { MEM_ALLOCATED, MEM_FREE };

typedef enum MEM_TYPE MEM_TYPE;

typedef struct memory_block_meta memory_block_meta;
struct memory_block_meta {
  size_t size;    // unsiged long type do not include meta data size
  MEM_TYPE type;  // specfiy which type
  memory_block_meta * nextBlock;
  memory_block_meta * prevBlock;
  void * data;  // start address of this memory block
};

struct memory_control_block {
  void * freeListHead;
  size_t heap_size;
};

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct memory_control_block memory_control_block;
memory_control_block * block_manager = NULL;

// __thread memory_control_block * block_manager_thread_local = NULL;

void * ts_malloc_lock(size_t size);
void ts_free_lock(void * ptr);

// void * ts_malloc_nolock(size_t size);
// void ts_free_nolock(void * ptr);

void init_memory_control_block(memory_control_block ** block_manager);
void * bf_malloc(size_t size, memory_control_block ** block_manager);
void * bf_getBlock(size_t size, memory_control_block * block_manager);
void bf_free(void * ptr, memory_control_block * block_manager);
void * insertToList(memory_block_meta * toAdd, memory_control_block * block_manager);
void * removeFromList(memory_block_meta * toRemove, memory_control_block * block_manager);
void * getNewBlock(size_t size, memory_control_block * block_manager);
void * sliceChunk(memory_block_meta * chunk, size_t request);
void * mergeBlock(memory_block_meta * merged, memory_control_block * block_manager);
#endif
