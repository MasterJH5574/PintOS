#ifndef VM_FRAME_H
#define VM_FRAME_H
#include "threads/thread.h"
#include "hash.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "lib/stdbool.h"
#include "threads/vaddr.h"

struct frame_info{
    thread* thread_hold;
    void* page;
    void* frame;
    struct hash_elem elem;
    struct list_elem list_elem;
};
typedef struct frame_info frame_info;


void frame_init(void);
void* frame_get_frame(enum palloc_flags flag, void* user_page,
                      bool page_table_locked);
void frame_free_frame(void* frame);

#endif //VM_FRAME_H
