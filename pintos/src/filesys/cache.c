//
// Created by jinho on 6/26/2020.
//

#include "cache.h"
#include "threads/malloc.h"
#include <string.h>
#define CACHE_BLOCK_NUM 64

static hash lru_hash;
static list lru_list;
static int block_num;
static unsigned cache_hash_func(const hash_elem *e, void *aux) {
  cache_block *block = hash_entry(e, cache_block, hashElem);
  return hash_int(block->sector_no);
}
static bool cache_hash_less_func(const hash_elem *a, const hash_elem *b,
                                 void *aux) {
  return cache_hash_func(a, NULL) < cache_hash_func(b, NULL);
}

void  __attribute__((optimize("-O0")))cache_init() {
  list_init(&lru_list);
  hash_init(&lru_hash, cache_hash_func, cache_hash_less_func, NULL);
}

static cache_block *block_to_replace() {
  return list_entry(list_front(&lru_list), cache_block, listElem);
}
static cache_block *fetch_sector(block_sector_t sector) {
  cache_block *new_block;
  if (block_num < CACHE_BLOCK_NUM) {
    block_num++;
    new_block = (cache_block *)malloc(sizeof(cache_block));
    lock_init(&new_block->lock);
    lock_acquire(&new_block->lock);
  } else {
    new_block = block_to_replace();
    lock_acquire(&new_block->lock);
    if (new_block->dirty) {
      block_write(fs_device, new_block->sector_no, new_block->block);
    }
    list_pop_front(&lru_list);
    hash_delete(&lru_hash, &new_block->hashElem);
  }
  block_read(fs_device, sector, new_block->block);
  list_push_back(&lru_list, &new_block->listElem);
  new_block->sector_no = sector;
  new_block->dirty = false;
  hash_insert(&lru_hash, &new_block->hashElem);
  lock_release(&new_block->lock);
  return new_block;
}

void cache_read(block_sector_t sector, void *buffer) {
  cache_block tmp;
  tmp.sector_no = sector;
  hash_elem *found_elem = hash_find(&lru_hash, &tmp.hashElem);
  cache_block *block;
  if (found_elem == NULL) {
    block = fetch_sector(sector);
    lock_acquire(&block->lock);
  } else {
    block = hash_entry(found_elem, cache_block, hashElem);
    lock_acquire(&block->lock);
    list_remove(&block->listElem);
    list_push_back(&lru_list, &block->listElem);
  }
  memcpy(buffer, block->block, BLOCK_SECTOR_SIZE);
  lock_release(&block->lock);
}
void cache_write(block_sector_t sector, void *buffer) {
  cache_block tmp;
  tmp.sector_no = sector;
  hash_elem *found_elem = hash_find(&lru_hash, &tmp.hashElem);
  cache_block *block;
  if (found_elem == NULL) {
    block = fetch_sector(sector);
    lock_acquire(&block->lock);
  } else {
    block = hash_entry(found_elem, cache_block, hashElem);
    lock_acquire(&block->lock);
    list_remove(&block->listElem);
    list_push_back(&lru_list, &block->listElem);
  }
  block->dirty = true;
  memcpy(block->block, buffer, BLOCK_SECTOR_SIZE);
  lock_release(&block->lock);
}
void cache_flush() {
  for (list_elem *iter = list_begin(&lru_list); iter != list_end(&lru_list);
       iter = list_next(iter)) {
    cache_block *block = list_entry(iter, cache_block, listElem);
    lock_acquire(&block->lock);
    if (block->dirty) {
      block_write(fs_device, block->sector_no, block->block);
    }
    block->dirty = false;
    lock_release(&block->lock);
  }
}
