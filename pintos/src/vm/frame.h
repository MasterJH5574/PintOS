#ifndef VM_FRAME_H
#define VM_FRAME_H
#include "threads/thread.h"
#include "hash.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "lib/stdbool.h"
#include "threads/vaddr.h"

struct frame_info{
    list thread_list;
    hash thread_hash;
    bool access;
    void* page;
    void* frame;
    struct hash_elem elem;
    struct list_elem list_elem;
};
struct frame_thread{
  thread *value;
  struct list_elem list_e;
  struct hash_elem hash_e;
};
typedef struct frame_info frame_info;
typedef struct frame_thread frame_thread;


void frame_init(void);
void* frame_get_frame(enum palloc_flags flag, void* user_page);
void frame_free_frame(void* frame);
bool frame_add_thread(void *frame, thread *t);
bool frame_remove_thread(void *frame, thread *t);
list *frame_thread_list(void *frame);
hash *frame_thread_hash(void *frame);

#endif //VM_FRAME_H
