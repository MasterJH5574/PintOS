//
// Created by jinho on 6/26/2020.
//

#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H
#include "devices/block.h"
#include "filesys.h"
#include "hash.h"
#include "list.h"
#include "threads/synch.h"
typedef struct cache_block {
  list_elem listElem;
  hash_elem hashElem;
  char block[BLOCK_SECTOR_SIZE];
  block_sector_t sector_no;
  bool dirty;
  lock_t lock;
} cache_block;

void cache_read(block_sector_t sector, void *buffer);
void cache_write(block_sector_t sector, void *buffer);

void cache_init(void);
void cache_flush(void);

#endif // FILESYS_CACHE_H
