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
unsigned hash_thread(const hash_elem *e, void *aux UNUSED);
bool hash_thread_less(const hash_elem *a, const hash_elem *b, void *aux UNUSED);

unsigned hash_frame(const hash_elem* e, void* aux UNUSED){
    frame_info* info=hash_entry(e,frame_info,elem);
    return hash_int((int) info->frame);
}

bool hash_frame_less(const hash_elem* a,const hash_elem* b, void * aux UNUSED){
    return hash_frame(a,NULL)<hash_frame(b,NULL);
}
/* ZYHowell: frame hashlist*/
unsigned hash_thread(const hash_elem *e, void *aux UNUSED) {
  frame_thread *tmp = hash_entry(e, frame_thread, hash_e);
  return hash_int((int) tmp->value);
}
bool hash_thread_less(const hash_elem *a, const hash_elem *b, void *aux UNUSED) {
  return hash_thread(a, NULL) < hash_thread(b, NULL);
}
/* hashlist support end*/

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
  new_info->access = true;

  list_init(&new_info->thread_list);
  hash_init(&new_info->thread_hash, hash_thread, hash_thread_less, NULL);
  frame_thread *ft = malloc(sizeof(frame_thread));
  ft->value = thread_current();
  list_push_back(&new_info->thread_list, &ft->list_e);
  hash_insert(&new_info->thread_hash, &ft->hash_e);

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
  for (list_elem *e = list_begin(&info->thread_list);
      e != list_end(&info->thread_list);e = list_next(e)) {
        free(list_entry(e, struct frame_thread, list_e));
      }
  
  free(info);
  palloc_free_page(frame);
  lock_release(&mutex);
}

/* ZYHowell: support a frame shared by multi-threads*/
bool frame_add_thread(void *frame, thread *t) {
  lock_acquire(&mutex);
  frame_info tmp, *info;
  struct hash_elem *elem;
  tmp.frame = frame;
  elem = hash_find(&frame_table, &tmp.elem);
  if (elem == NULL) PANIC("invalid frame");
  info = hash_entry(elem, struct frame_info, elem);

  frame_thread *th = malloc(sizeof(frame_thread));
  th->value = t;
  hash_insert(&info->thread_hash, &th->hash_e);
  list_push_back(&info->thread_list, &th->list_e);
  lock_release(&mutex);
  return true;
}
bool frame_remove_thread(void *frame, thread *t) {
  lock_acquire(&mutex);
  frame_info tmp, *info;
  frame_thread thtmp, *thInfo;
  struct hash_elem *elem;
  tmp.frame = frame;
  elem = hash_find(&frame_table, &tmp.elem);
  if (elem == NULL) PANIC("invalid frame");
  info = hash_entry(elem, struct frame_info, elem);
  thtmp.value = t;
  elem = hash_find(&info->thread_hash, &thtmp.hash_e);
  if (elem == NULL) {
    return false; //no such thread in the frame, maybe a panic?
  }
  thInfo = hash_entry(elem, struct frame_thread, hash_e);
  hash_delete(&info->thread_hash, &thInfo->hash_e);
  list_remove(&thInfo->list_e);
  free(thInfo);
  lock_release(&mutex);
  if (list_empty(&info->thread_list)) frame_free_frame(frame);
  return true;
}
list *frame_thread_list(void *frame) {
  lock_acquire(&mutex);
  struct frame_info tmp, *info;
  struct hash_elem *elem;
  tmp.frame = frame;
  elem = hash_find(&frame_table, &tmp.elem);
  if (elem == NULL) PANIC("invalid frame");
  info = hash_entry(elem, struct frame_info, elem);
  lock_release(&mutex);
  return &info->thread_list;
}
hash *frame_thread_hash(void *frame) {
  lock_acquire(&mutex);
  struct frame_info tmp, *info;
  struct hash_elem *elem;
  tmp.frame = frame;
  elem = hash_find(&frame_table, &tmp.elem);
  if (elem == NULL) PANIC("invalid frame");
  info = hash_entry(elem, struct frame_info, elem);
  lock_release(&mutex);
  return &info->thread_hash;
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
    if (list_cur->access) {
      list_cur->access = false;
      cur_next();
    } else {
      list_elem *elem = list_begin(&list_cur->thread_list);
      frame_thread *ft = list_entry(elem, struct frame_thread, list_e);
      pte = pte_find(&ft->value->page_table, list_cur->page, false);
      /* If the page was pinned, or it belongs to other thread, it cannot
       * be chosen.
       */
      if (pte->pinned)
        cur_next();
      else {
        //if (pte->status != FRAME) ASSERT(false);
        break;
      }
    }
  }
  frame_info *tmp = list_cur;
  void *rep_frame = list_cur->frame;

  list_elem *elem = list_begin(&list_cur->thread_list);
  frame_thread *ft = list_entry(elem, struct frame_thread, list_e);
  pte = pte_find(&ft->value->page_table, list_cur->page, false);

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
  for (list_elem *e = list_begin(&list_cur->thread_list);
      e != list_end(&list_cur->thread_list);e = list_next(e)) {
        thread *thread_hold = list_entry(e, struct frame_thread, list_e)->value;
        pagedir_clear_page(thread_hold->pagedir, list_cur->page);
      }
  //pagedir_clear_page(list_cur->thread_hold->pagedir, list_cur->page);

  //below: remove it from frame list and frame hash
  cur_next();
  if (list_cur == tmp) list_cur = NULL;
  list_remove(&tmp->list_elem);
  hash_delete(&frame_table, &tmp->elem);
  free(tmp);
  return rep_frame;
}