#include "frame.h"
#include "swap.h"
#include "userprog/pagedir.h"
#include <string.h>
static struct lock mutex;
static hash frame_table;
static list frame_list;
struct frame_info* list_cur;

/* Ruihang Begin: Declarations for eviction algorithm. */
unsigned hash_frame(const hash_elem *e, void *aux);
bool hash_frame_less(const hash_elem *a, const hash_elem *b, void *aux);

void *replace2get_page(void);
void cur_next(void);
/* Ruihang End */

unsigned hash_frame(const hash_elem* e, void* aux UNUSED){
    frame_info* info=hash_entry(e,frame_info,elem);
    return hash_int((int) info->frame);
}

bool hash_frame_less(const hash_elem* a,const hash_elem* b, void * aux UNUSED){
    return hash_frame(a,NULL)<hash_frame(b,NULL);
}


void __attribute__((optimize("-O0"))) frame_init () {
    hash_init(&frame_table,hash_frame,hash_frame_less,NULL);
    list_init(&frame_list);
    list_cur = NULL;
    lock_init(&mutex);
}

void *frame_get_frame (enum palloc_flags flag, void *user_page) {
  user_page = pg_round_down(user_page);
  lock_acquire(&mutex);
  void* frame = palloc_get_page(flag|PAL_USER);

  if (frame == NULL) {
    if (flag & PAL_ASSERT) PANIC("try to get frame but pages run out, assert not to replace");
    frame = replace2get_page();
    if (flag & PAL_ZERO)
      memset(frame, 0, PGSIZE);
  }
  if (frame == NULL) {
    lock_release(&mutex);
    PANIC("something wrong. ");
  }

  frame_info* new_info=(frame_info*)malloc(sizeof(frame_info));
  new_info->frame=frame;
  new_info->page=user_page;
  new_info->thread_hold=thread_current();

  hash_insert(&frame_table, &new_info->elem);

  if (list_empty(&frame_list)) list_cur = new_info;
  list_push_back(&frame_list, &new_info->list_elem);
  lock_release(&mutex);
  return frame;
}

void frame_free_frame(void* frame) {
    lock_acquire(&mutex);

    struct frame_info tmp, *info;
    struct hash_elem *elem;
    tmp.frame = frame;
    elem = hash_find(&frame_table, &tmp.elem);
    if (elem == NULL) PANIC("invalid frame to free");
    info = hash_entry(elem, struct frame_info, elem);
    
    if (list_cur == info) {
      cur_next();
      if (list_cur == info) list_cur = NULL;  //it is actually the next now, but as it not changed, the list contains only one element and is going to be removed. So NULL. 
    }
    list_remove(&info->list_elem);

    hash_delete(&frame_table, &info->elem);
    free(info);
    palloc_free_page(frame);
    lock_release(&mutex);
}

/* ZYHowell: page replacement algorithm: CLOCK*/
void cur_next() {
  if (&list_cur->list_elem == list_back(&frame_list))
      list_cur = list_entry(list_head(&frame_list), struct frame_info, list_elem);
  list_cur = list_entry(list_next(&list_cur->list_elem), struct frame_info, list_elem);
}
void *replace2get_page() {
  struct page_table_entry *pte = NULL;
  while(true){
    if (pagedir_is_accessed(list_cur->thread_hold->pagedir, list_cur->page)) {
      pagedir_set_accessed(list_cur->thread_hold->pagedir, list_cur->page,
                           false);
      cur_next();
    } else {
      pte = pte_find(&list_cur->thread_hold->page_table, list_cur->page,
                     false);
      /* If the page was pinned, or it belongs to other thread, it cannot
       * be chosen.
       */
      if (pte->pinned)
        //|| (list_cur->thread_hold != thread_current() && pte->status == FILE))
        cur_next();
      else {
        if (pte->status != FRAME) ASSERT(false);
        break;
      }
    }
  }
  frame_info *tmp = list_cur;
  void *rep_frame = list_cur->frame;
  pte = pte_find(&list_cur->thread_hold->page_table,
                 list_cur->page, false);
//  ASSERT(pte->status == FRAME)
  if (pte->file != NULL) {
    page_table_mmap_write_back(pte);
    pte->status = FILE;
    pte->frame = NULL;
  }
  else {
    block_sector_t index = swap_store(rep_frame);
    if (index == ((block_sector_t) -1)) return NULL;//something wrong
    pte->swap_index = index;
    pte->status = SWAP;
    pte->frame = NULL;
  }
  pagedir_clear_page(list_cur->thread_hold->pagedir, list_cur->page);

  //below: remove it from frame list and frame hash
  cur_next();
  if (list_cur == tmp) list_cur = NULL;
  list_remove(&tmp->list_elem);
  hash_delete(&frame_table, &tmp->elem);
  free(tmp);
  return rep_frame;
}