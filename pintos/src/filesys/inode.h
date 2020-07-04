#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "devices/block.h"
#include "filesys/cache.h"
#include "threads/thread.h"
struct bitmap;
#define PTR_NUM_BLOCK 127
typedef struct inode_disk
{
  block_sector_t start;               /* First data sector. */
  off_t length;                       /* File size in bytes. */
  unsigned magic;                     /* Magic number. */
  uint32_t unused[124];               /* Not used. */
  bool isdir;                         /* Is dir file. */
} inode_disk;
typedef struct inode_table_disk{
  block_sector_t ptr[PTR_NUM_BLOCK];
  block_sector_t next_block;
}inode_table_disk;
/* In-memory inode. */
struct inode
{
  struct list_elem elem;              /* Element in inode list. */
  block_sector_t sector;              /* Sector number of disk location. */
  int open_cnt;                       /* Number of openers. */
  bool removed;                       /* True if deleted, false otherwise. */
  int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
  struct inode_disk data;             /* Inode content. */
  list threads_open;
};
void inode_init (void);
bool inode_create (block_sector_t, off_t);
struct inode *inode_open (block_sector_t);
struct inode *inode_reopen (struct inode *);
block_sector_t inode_get_inumber (const struct inode *);
void inode_close (struct inode *);
void inode_remove (struct inode *);
off_t inode_read_at (struct inode *, void *, off_t size, off_t offset);
off_t inode_write_at (struct inode *, const void *, off_t size, off_t offset);
void inode_deny_write (struct inode *);
void inode_allow_write (struct inode *);
off_t inode_length (const struct inode *);

bool inode_is_root(struct inode *inode);
bool inode_isdir(struct inode*);
void inode_set_dir(struct inode*);
int inode_get_opencnt(struct inode* inode);
void inode_add_thread_open(struct inode*, struct thread*);
//struct thread* inode_get_open_thread(struct inode* inode);

#endif /* filesys/inode.h */
