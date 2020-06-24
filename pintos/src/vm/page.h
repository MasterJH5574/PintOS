#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>

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
  void *frame_number;                     /* As the value of page table. */

  enum page_status status;                /* Show where the page is. */

  // Todo: maybe some other members are needed.

  struct hash_elem elem;
};


/* Methods related to page table. */
void page_table_init(page_table_t *page_table);



/* Ruihang End */

#endif //VM_PAGE_H
