#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include <filesys/off_t.h>

/* Ruihang Begin */
typedef struct hash page_table_t;
typedef int mapid_t;

/* Type page_status represents where the page is. */
enum page_status {
  FRAME,
  SWAP,
  FILE
};

struct page_table_entry {
  void *upage;                            /* As the key of page table. */
  enum page_status status;                /* Show where the page is. */
  bool writable;                          /* Whether this page is writable. */


  void *frame;                            /* If status == FRAME. */

  uint32_t swap_index;                    /* If status == SWAP. */
  // Actually, "uint32_t" should be swap_index_t after implementing swap.

  struct file *file;                      /* If status == FILE. */
  off_t file_offset;
  uint32_t read_bytes;
  uint32_t zero_bytes;


  // Todo: maybe this structure will be modified again and again.

  struct hash_elem elem;
};


struct mmap_descriptor {
  mapid_t mapid;
  struct page_table_entry *pte;
  struct list_elem elem;
};


/* Methods related to page table. */
void page_table_lock_init(void);

bool page_table_init(page_table_t *page_table);
void page_table_destroy(page_table_t *page_table);
bool page_table_set_page(void *upage, void *kpage, bool writable);
void page_table_remove_page(struct page_table_entry *pte);
struct page_table_entry *pte_find(page_table_t *page_table,
                                  void *upage,
                                  bool locked);

bool page_table_map_file_page(struct file *file,
                              off_t ofs,
                              uint32_t *upage,
                              uint32_t read_bytes,
                              uint32_t zero_bytes,
                              bool writable);
void page_table_remove_mmap(struct list *mmap_descriptors, mapid_t mapping);

bool page_fault_handler(const void *fault_addr, bool write, void *esp);


/* Ruihang End */

#endif //VM_PAGE_H
