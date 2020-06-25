#include "vm/page.h"
#include <lib/debug.h>
#include <userprog/pagedir.h>
#include <threads/thread.h>
#include <threads/malloc.h>
#include <threads/vaddr.h>

/* Ruihang Begin */
static struct lock page_table_lock;
void
page_table_lock_init() {
  lock_init(&page_table_lock);
}


unsigned page_table_hash_hash_func(const struct hash_elem *e, void *aux);
bool page_table_hash_less_func(const struct hash_elem *a,
                               const struct hash_elem *b,
                               void *aux);
void page_table_entry_destroy(struct hash_elem *e, void *aux);

bool is_stack_access(const void *vaddr, const void *esp);


/* Initialize supplemental page table. */
bool
page_table_init(page_table_t *page_table) {
  return hash_init(page_table,
                   page_table_hash_hash_func,
                   page_table_hash_less_func,
                   NULL);
}

/* Destroy the supplemental page table in thread_exit(). */
void
page_table_destroy(page_table_t *page_table) {
  hash_destroy(page_table, page_table_entry_destroy);
}

/* Hash hash function of supplemental page table. (type hash_hash_func) */
unsigned
page_table_hash_hash_func(const struct hash_elem *e, void *aux UNUSED) {
  struct page_table_entry *pte = hash_entry(e, struct page_table_entry, elem);
  return hash_int((int) pte->page_number);
}

/* Hash less function of supplemental page table. (type hash_less_func) */
bool
page_table_hash_less_func(const struct hash_elem *a,
                          const struct hash_elem *b,
                          void *aux UNUSED) {
  struct page_table_entry *pte_a = hash_entry(a, struct page_table_entry, elem);
  struct page_table_entry *pte_b = hash_entry(b, struct page_table_entry, elem);
  return pte_a->page_number < pte_b->page_number;
}

/* Destroy supplemental page table entry. */
void
page_table_entry_destroy(struct hash_elem *e, void *aux UNUSED) {
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

/* Find the spte of user_page_number in page_table. */
struct page_table_entry *
pte_find(page_table_t *page_table, void *user_page_number, bool locked) {
  if (!locked)
    lock_acquire(&page_table_lock);
  ASSERT(page_table != NULL)

  struct page_table_entry key;
  key.page_number = user_page_number;
  struct hash_elem *elem = hash_find(page_table, &key.elem);

  if (!locked)
    lock_release(&page_table_lock);

  return elem != NULL ? hash_entry(elem, struct page_table_entry, elem) : NULL;
}
/* Ruihang End */

/*Jiaxin Begin*/

/*
* Given a virtual address(page) and a hashtable, allocate a frame for this page,
* return whether successful or not
*/
bool
page_fault_handler(const void *fault_addr, bool write, void *esp)
{
  ASSERT(is_user_vaddr(fault_addr))

  struct thread *cur = thread_current();
  page_table_t *page_table = &cur->page_table;
  void *user_page_number = pg_round_down(fault_addr);

  bool success = true;
  lock_acquire(&page_table_lock);

  struct page_table_entry *pte = pte_find(page_table, user_page_number, true);
  ASSERT(!(pte != NULL && pte->status == FRAME))

  /* If the page is read-only, return false. */
  if (write == true && pte != NULL && pte->writable == false)
    return false;

  //A lot more && might need something from frame!

  /* Ruihang Begin */
  if (is_stack_access(fault_addr, esp)) {
    /* The access to fault_addr is a stack access. */
    /* Maybe need to perform some stack-growth? */
    // Todo
  } else {
    /* The access to fault_addr is not a stack access. */
    /* Only pte != NULL can be handled. Otherwise success = false. */
    if (pte != NULL) {
      /* Note that pte->status != FRAME by the assertion above. */
      if (pte->status == SWAP) {
        // Todo
      } else if (pte->status == FILE) {
        // Todo
      } else
        ASSERT(false)
    } // else success = false
  }
  /* Ruihang End */

  // Todo

  return success;
}
/*Jiaxin End*/

/* Ruihang Begin */
bool
is_stack_access(const void *vaddr, const void *esp) {
  /* The 80x86 PUSH instruction checks access permissions before it adjusts
   * the stack pointer, so it may cause a page fault 4 bytes below the
   * stack pointer.                           ------5.3.3 Stack Growth
   */
  return vaddr >= esp - 4 * 8;
}
/* Ruihang End */
