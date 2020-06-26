#include "swap.h"
#include "threads/pte.h"
#include "threads/malloc.h"
#include <hash.h>

/*Jiaxin Begin*/

const int BLOCK_NUM_PER_PAGE = PGSIZE / BLOCK_SECTOR_SIZE;

struct swap_item
{
  block_sector_t index;
  struct list_elem list_elem;
};

struct block *swap_block;
static struct list swap_free_list;
block_sector_t swap_top;

/*Add a free swap slot*/
void swap_add_free_slot(block_sector_t index);

/*Get a free swap slot*/
block_sector_t swap_get_free_slot();

/*Initialize swap when kernel starts*/
void
swap_init()
{
  swap_block = block_get_role(BLOCK_SWAP);
  ASSERT(swap_block != NULL);
  list_init(&swap_free_list);
}

/* Store a KPAGE to a swap slot
* return a block_sector_t index for later use
*/
block_sector_t
swap_store(void *kpage)
{
  ASSERT(is_kernel_vaddr(kpage));
  block_sector_t index = swap_get_free_slot();
  if (index == ((block_sector_t) -1))
    return index;
  for (int i = 0; i < BLOCK_NUM_PER_PAGE; ++i)
  {
    block_write(swap_block, index + i, kpage + i * BLOCK_SECTOR_SIZE);
  }
  return index;
}

/* Load a swap slot into a KPAGE
*  index should come from the return of a swap_store
*/
void
swap_load(block_sector_t index, void *kpage)
{
  ASSERT(index != ((block_sector_t) -1));
  ASSERT(is_kernel_vaddr(kpage));
  ASSERT(index % BLOCK_NUM_PER_PAGE == 0);

  for (int i = 0; i < BLOCK_NUM_PER_PAGE; ++i)
  {
    block_read(swap_block, index + i, kpage + i * BLOCK_SECTOR_SIZE);
  }
  swap_free(index);
}

/* Free a swap slot whose identifier is index 
*  which is obtained from swap_store
*/
void swap_free(block_sector_t index)
{
  ASSERT(index % BLOCK_NUM_PER_PAGE == 0);

  //At the top of swap slot so just move the swap_top
  if (index + BLOCK_NUM_PER_PAGE == swap_top)
  {
    swap_top = index;
  } else {
    //In the middle of swap slot, add into swap_free_list
    swap_add_free_slot(index);
  }
}

/*Add a free swap slot*/
void
swap_add_free_slot(block_sector_t index)
{
  struct swap_item* tmp = malloc(sizeof(struct swap_item));
  tmp->index = index;
  list_push_back(&swap_free_list, &tmp->list_elem);
}

/*Get a free swap slot*/
block_sector_t
swap_get_free_slot()
{
  block_sector_t ret = (block_sector_t) -1;
  //No free slot
  if (list_empty(&swap_free_list))
  {
    if (swap_top + BLOCK_NUM_PER_PAGE < block_size(swap_block))
    {
      ret = swap_top;
      swap_top += BLOCK_NUM_PER_PAGE;
    }
  } else {
    //Has a free slot
    struct swap_item* tmp = list_entry(list_front(&swap_free_list),struct swap_item, list_elem);
    list_remove(&tmp->list_elem);
    ret = tmp->index;
    free(tmp);
  }
  return ret;
}

/*Jiaxin End*/
