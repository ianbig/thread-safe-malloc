#include "my_malloc.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void * ts_malloc_lock(size_t size) {
  pthread_mutex_lock(&lock);
  void * ret_addr = bf_malloc(size);
  pthread_mutex_unlock(&lock);

  return ret_addr;
}
void ts_free_lock(void * ptr) {
  pthread_mutex_lock(&lock);
  bf_free(ptr);
  pthread_mutex_unlock(&lock);
}

void init_memory_control_block() {
  // initialized memory for block manager
  block_manager = sbrk(sizeof(memory_control_block));
  block_manager->freeListHead = NULL;
}

void * bf_malloc(size_t size) {
  if (block_manager == NULL) {
    init_memory_control_block();
  }
  // find free list for first freed block
  void * chunk = bf_getBlock(size);
  return chunk;
}

void * bf_getBlock(size_t size) {
  memory_block_meta * freePtr = block_manager->freeListHead;
  size_t best_size = INT_MAX;
  memory_block_meta * best_block = NULL;
  while (freePtr != NULL) {
    if (freePtr->size == size) {
      best_block = freePtr;
      break;
    }

    if (freePtr->size > size && best_size > freePtr->size) {
      best_block = freePtr;
      best_size = freePtr->size;
    }

    freePtr = freePtr->nextBlock;
  }

  if (best_block != NULL) {
    removeFromList(best_block);
    void * remainChunk = sliceChunk(best_block, size);
    if (remainChunk != NULL) {
      insertToList(remainChunk);
    }
  }

  else {
    best_block = getNewBlock(size + sizeof(memory_block_meta));
  }

  best_block->type = MEM_ALLOCATED;

  return best_block->data;
}

void * insertToList(memory_block_meta * toAdd) {
  assert(toAdd != NULL);
  memory_block_meta * curNode = block_manager->freeListHead;
  while (curNode != NULL && curNode->nextBlock != NULL && curNode < toAdd) {
    curNode = (curNode)->nextBlock;
  }

  if (curNode == NULL) {
    block_manager->freeListHead = toAdd;
    toAdd->nextBlock = NULL;
    toAdd->prevBlock = NULL;
  }

  else if (curNode == block_manager->freeListHead && toAdd < curNode) {
    toAdd->nextBlock = curNode;
    curNode->prevBlock = toAdd;
    toAdd->prevBlock = NULL;
    block_manager->freeListHead = toAdd;
  }

  else if (curNode->nextBlock == NULL && toAdd > curNode) {
    curNode->nextBlock = toAdd;
    toAdd->prevBlock = curNode;
    toAdd->nextBlock = NULL;
  }

  else {
    toAdd->nextBlock = curNode;
    toAdd->prevBlock = curNode->prevBlock;
    curNode->prevBlock = toAdd;
    toAdd->prevBlock->nextBlock = toAdd;
  }

  return toAdd;
}

void * removeFromList(memory_block_meta * toRemove) {
  if (toRemove == block_manager->freeListHead) {
    block_manager->freeListHead = toRemove->nextBlock;
  }
  if (toRemove->prevBlock != NULL) {
    toRemove->prevBlock->nextBlock = toRemove->nextBlock;
  }
  if (toRemove->nextBlock != NULL) {
    toRemove->nextBlock->prevBlock = toRemove->prevBlock;
  }

  toRemove->nextBlock = NULL;
  toRemove->prevBlock = NULL;

  return toRemove;
}

void * getNewBlock(size_t size) {
  memory_block_meta * newChunk = sbrk(size);
  newChunk->size = size - sizeof(*newChunk);
  newChunk->type = MEM_ALLOCATED;
  newChunk->nextBlock = NULL;
  newChunk->prevBlock = NULL;
  newChunk->data = (void *)newChunk + sizeof(memory_block_meta);
  block_manager->heap_size += size;

  return newChunk;
}

void * sliceChunk(memory_block_meta * chunk, size_t request) {
  assert(chunk != NULL);  // TODO: remove this
  size_t remaining_size = (chunk->size >= request) ? chunk->size - request : 0;
  if (remaining_size <= sizeof(*chunk)) {
    return NULL;
  }

  chunk->size = request;
  chunk->type = MEM_ALLOCATED;
  // chunk->nextBlock = NULL;
  memory_block_meta * remain_chunk = (void *)chunk + sizeof(*chunk) + request;
  remain_chunk->size = remaining_size - sizeof(*remain_chunk);
  remain_chunk->type = MEM_FREE;
  remain_chunk->nextBlock = NULL;
  remain_chunk->data = (void *)remain_chunk + sizeof(*remain_chunk);

  return remain_chunk;
}

void bf_free(void * toFree) {
  memory_block_meta * freeBlock = toFree - sizeof(memory_block_meta);
  assert(block_manager != NULL);

  if (freeBlock->type == MEM_FREE) {
    fprintf(stderr, "Error: double free at adress %p\n", freeBlock);
    exit(EXIT_FAILURE);
  }

  freeBlock->type = MEM_FREE;
  freeBlock = insertToList(freeBlock);
  mergeBlock(freeBlock);
}

void * mergeBlock(memory_block_meta * merged) {
  memory_block_meta * prev_block_end = NULL;
  memory_block_meta * prev_block_start = merged->prevBlock;
  if (merged->prevBlock != NULL) {
    prev_block_end =
        (void *)prev_block_start + sizeof(*prev_block_end) + prev_block_start->size;
  }

  memory_block_meta * next_block_start = merged->nextBlock;

  memory_block_meta *freeBlock = NULL, *curNode = NULL;  // TODO: remove this

  if (next_block_start != NULL &&
      (void *)merged + sizeof(*merged) + merged->size == next_block_start) {
    merged->size += sizeof(*next_block_start) + next_block_start->size;
    removeFromList(next_block_start);

    // TODO: remove this
    freeBlock = merged;
    curNode = next_block_start;
  }

  if (prev_block_start != NULL &&
      (void *)prev_block_start + sizeof(*prev_block_start) + prev_block_start->size ==
          merged) {
    prev_block_start->size += sizeof(*merged) + merged->size;
    removeFromList(merged);

    // TODO: remove this
    curNode = merged;
    freeBlock = prev_block_start;
  }

  return NULL;
}
