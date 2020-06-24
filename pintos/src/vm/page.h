#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include <filesys/off_t.h>

/* Ruihang Begin */
typedef struct hash page_table_t;

/* Type page_status represents where the page is. */
enum page_status {
  FRAME,
  SWAP,
  FILE
};

struct page_table_entry {
  void *page_number;                      /* As the key of page table. */
  enum page_status status;                /* Show where the page is. */


  void *frame_number;                     /* If status == FRAME. */

  uint32_t swap_index;                    /* If status == SWAP. */
  // Actually, "uint32_t" should be swap_index_t after implementing swap.

  struct file *file;                      /* If status == FILE. */
  off_t file_offset;
  uint32_t read_bytes;
  uint32_t zero_bytes;
  bool writable;


  // Todo: maybe this structure will be modified again and again.

  struct hash_elem elem;
};


/* Methods related to page table. */
void page_table_lock_init(void);

bool page_table_init(page_table_t *page_table);
void page_table_destroy(page_table_t *page_table);
struct page_table_entry *pte_find(page_table_t *page_table,
                                  void *user_page_number);



/* Ruihang End */

#endif //VM_PAGE_H
