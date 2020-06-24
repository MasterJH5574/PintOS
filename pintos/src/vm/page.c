#include "vm/page.h"
#include <lib/debug.h>
#include <userprog/pagedir.h>
#include <threads/thread.h>
#include <threads/malloc.h>

/* Ruihang Begin */
unsigned page_table_hash_hash_func(const struct hash_elem *e, void *aux);
bool page_table_hash_less_func(const struct hash_elem *a,
                               const struct hash_elem *b,
                               void *aux);
void page_table_entry_destroy(struct hash_elem *e, void *aux);


/* Initialize page table. */
bool
page_table_init(page_table_t *page_table) {
  return hash_init(page_table,
                   page_table_hash_hash_func,
                   page_table_hash_less_func,
                   NULL);
}

/* Destroy the page table in thread_exit(). */
void
page_table_destroy(page_table_t *page_table) {
  hash_destroy(page_table, page_table_entry_destroy);
}

/* Hash hash function of page table. (type hash_hash_func) */
unsigned
page_table_hash_hash_func(const struct hash_elem *e, void *aux) {
  struct page_table_entry *pte = hash_entry(e, struct page_table_entry, elem);
  return hash_int((int) pte->page_number);
}

/* Hash less function of page table. (type hash_less_func) */
bool
page_table_hash_less_func(const struct hash_elem *a,
                          const struct hash_elem *b,
                          void *aux) {
  struct page_table_entry *pte_a = hash_entry(a, struct page_table_entry, elem);
  struct page_table_entry *pte_b = hash_entry(b, struct page_table_entry, elem);
  return pte_a->page_number < pte_b->page_number;
}

/* Destroy page table entry. */
void
page_table_entry_destroy(struct hash_elem *e, void *aux) {
  struct page_table_entry *pte = hash_entry(e, struct page_table_entry, elem);
  if (pte->status == FRAME) {
    pagedir_clear_page(thread_current()->pagedir, pte->page_number);
    // Todo: frame_free_frame(pte->frame_number); after frame.h finished.
  } else if (pte->status == SWAP) {
    // Todo: maybe need something.
  } else if (pte->status == FILE) {
    // Todo: maybe need something, maybe not.
  } else {
    ASSERT(false)
  }
  free(pte);
}

/* Ruihang End */