#include "vm/page.h"

/* Ruihang Begin */
unsigned page_table_hash_hash_func(const struct hash_elem *e, void *aux);
bool page_table_hash_less_func(const struct hash_elem *a,
                               const struct hash_elem *b,
                               void *aux);
void
page_table_init(page_table_t *page_table) {
  hash_init(page_table,
            page_table_hash_hash_func,
            page_table_hash_less_func,
            NULL);
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

/* Ruihang End */