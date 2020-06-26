#include "page.h"
#include "frame.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include "threads/vaddr.h"

/* Ruihang Begin */
static struct lock page_table_lock;
void __attribute__((optimize("-O0")))
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
  return hash_int((int) pte->upage);
}

/* Hash less function of supplemental page table. (type hash_less_func) */
bool
page_table_hash_less_func(const struct hash_elem *a,
                          const struct hash_elem *b,
                          void *aux UNUSED) {
  struct page_table_entry *pte_a = hash_entry(a, struct page_table_entry, elem);
  struct page_table_entry *pte_b = hash_entry(b, struct page_table_entry, elem);
  return pte_a->upage < pte_b->upage;
}

/* Destroy supplemental page table entry. */
void
page_table_entry_destroy(struct hash_elem *e, void *aux UNUSED) {
  struct page_table_entry *pte = hash_entry(e, struct page_table_entry, elem);
  if (pte->status == FRAME) {
    pagedir_clear_page(thread_current()->pagedir, pte->upage);
    frame_free_frame(pte->frame);
  } else if (pte->status == SWAP) {
    // Todo: maybe need something.
  } else if (pte->status == FILE) {
    // Todo: maybe need something, maybe not.
  } else {
    ASSERT(false)
  }
  free(pte);
}

/* Adds a mapping from user virtual page UPAGE to the physical frame
 * identified by kernel virtual address KPAGE.
 *  * UPAGE must not already be mapped.
 *  * KPAGE should probably be a page obtained from the user pool with
 *    palloc_get_page().
 * If WRITABLE is true, the new page is read/write;
 * otherwise it is read-only.
 * Returns true if successful, false if memory allocation
 * failed.
 */
bool
page_table_set_page(void *upage, void *kpage, bool writable) {
  struct thread *t = thread_current();
  page_table_t *page_table = &t->page_table;
  uint32_t *pagedir = t->pagedir;

  bool success = false;
  lock_acquire(&page_table_lock);

  struct page_table_entry *pte = pte_find(page_table, upage, true);
  if (pte == NULL) {
    /* UPAGE not found. SUCCESS = true. */
    success = true;

    /* Perform set page: insert a new pte to supplemental page table. */
    pte = malloc(sizeof(struct page_table_entry));
    pte->upage = upage;
    pte->status = FRAME;

    pte->frame = kpage;
    pte->writable = writable;
    pte->swap_index = 0;
    pte->file = NULL;
    pte->file_offset = 0;
    pte->read_bytes = pte->zero_bytes = 0;

    hash_insert(page_table, &pte->elem);
  }

  lock_release(&page_table_lock);

  if (success) {
    bool pagedir_set_result = pagedir_set_page(pagedir, upage, kpage, writable);
    ASSERT(pagedir_set_result)
  }
  return success;
}

/* Remove the page/pte from supplemental page table. And then free PTE. */
void
page_table_remove_page(struct page_table_entry *pte) {
  hash_delete(&thread_current()->page_table, &pte->elem);
  free(pte);
}

/* Find the spte of UPAGE in page_table. Return NULL if upage not found. */
struct page_table_entry *
pte_find(page_table_t *page_table, void *upage, bool locked) {
  if (!locked)
    lock_acquire(&page_table_lock);
  ASSERT(page_table != NULL)
  /* Assert that UPAGE is page-aligned. */
  ASSERT((uint32_t) upage % (PGSIZE) == 0)

  struct page_table_entry key;
  key.upage = upage;
  struct hash_elem *elem = hash_find(page_table, &key.elem);

  if (!locked)
    lock_release(&page_table_lock);

  return elem != NULL ? hash_entry(elem, struct page_table_entry, elem) : NULL;
}

/* Map a page from file to user address UPAGE. */
bool
page_table_map_file_page(struct file *file,
                              off_t ofs,
                              uint32_t *upage,
                              uint32_t read_bytes,
                              uint32_t zero_bytes,
                              bool writable) {
  /* Assert that UPAGE is page-aligned. */
  ASSERT((uint32_t) upage % (PGSIZE) == 0)

  /* Create a new supplemental page table entry. */
  struct page_table_entry *pte = malloc(sizeof(struct page_table_entry));
  pte->upage = upage;
  pte->status = FILE;
  pte->writable = writable;
  pte->frame = NULL;
  pte->swap_index = 0;
  pte->file = file;
  pte->file_offset = ofs;
  pte->read_bytes = read_bytes;
  pte->zero_bytes = zero_bytes;

  struct mmap_descriptor *md = malloc(sizeof(struct mmap_descriptor));
  md->mapid = thread_current()->md_num;
  md->pte = pte;
  list_push_back(&thread_current()->mmap_descriptors, &md->elem);

  /* If insertion is successful, the result is NULL.
   * If an equal element is already in the table, result of hash_insert will
   * not be NULL, which means the insertion failed.
   */
  return hash_insert(&thread_current()->page_table, &pte->elem) == NULL;
}

/* Write back in-frame page back into mmapped file. */
void
page_table_mmap_write_back(struct page_table_entry *pte) {
  ASSERT(pte->status == FRAME)            /* The page is now in memory. */
  ASSERT(pte->file != NULL)               /* The page is a mmapped page. */
  uint32_t *pagedir = thread_current()->pagedir;

  /* If the page is clean, just return. */
  if (!pagedir_is_dirty(pagedir, pte->upage))
    return;

  lock_acquire(&filesys_lock);
  file_write_at(pte->file, pte->upage, pte->read_bytes, pte->file_offset);
  lock_release(&filesys_lock);
}

/* Remove mmapped pages designated by MAPPING  from supplemental page table. */
void
page_table_remove_mmap(struct list *mmap_descriptors, mapid_t mapping) {
  struct list_elem *e, *next;
  struct file *file_to_close = NULL;
  uint32_t *pagedir = thread_current()->pagedir;

  for (e = list_begin(mmap_descriptors);
        e != list_end(mmap_descriptors);
        e = next) {
    next = list_next(e);
    struct mmap_descriptor *md = list_entry(e, struct mmap_descriptor, elem);
    if (md->mapid != mapping)
      continue;

    ASSERT(md->pte->file != NULL)
    /* If FILE_TO_CLOSE is not set, set it so that it can ba closed later. */
    if (file_to_close == NULL)
      file_to_close = md->pte->file;
    else
      ASSERT(md->pte->file == file_to_close)

    /* If the corresponding page is in memory, check whether write-back
     * is needed.
     */
    if (md->pte->status == FRAME) {
      /* Write back to mmapped file. */
      page_table_mmap_write_back(md->pte);
      /* Release the page from page table. */
      frame_free_frame(md->pte->frame);
      pagedir_clear_page(pagedir, md->pte->upage);
    }

    /* Remove the page from supplemental page table. */
    page_table_remove_page(md->pte);
    list_remove(e);

    /* The mmap descriptor MD is no longer needed. Free it. */
    free(md);
  }


  /* Close the re-opened file of mmap. */
  lock_acquire(&filesys_lock);
  if (file_to_close != NULL)
    file_close(file_to_close);
  lock_release(&filesys_lock);
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
  void *introduced = NULL;

  bool success = false;
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
    /* ZYHowell: handle stack-growth*/
    if (pte != NULL) {
      if (pte->status != SWAP) success = false;
      introduced = frame_get_frame(0, user_page_number);
      if (introduced == NULL) success = false;
      else {
        //TODO: some command of swap here. 
      }
    } else {
      introduced = frame_get_frame(0, user_page_number);
      if (introduced == NULL) success = false;
      else {
        struct page_table_entry *tmp = malloc(sizeof(struct page_table_entry));
        tmp->upage = user_page_number;
        tmp->status = FRAME;
        tmp->frame = introduced;
        tmp->writable = true;
        tmp->frame = NULL;
        tmp->swap_index = 0;
        tmp->file = NULL;
        tmp->file_offset = 0;
        tmp->read_bytes = tmp->zero_bytes = 0;
        hash_insert(page_table, &tmp->elem);
      }
    }
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
