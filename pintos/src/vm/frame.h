//
// Created by jinho on 6/24/2020.
//
#include "threads/thread.h"
#include "hash.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#ifndef VM_FRAME_H
#define VM_FRAME_H

struct frame_info{
    thread* thread_hold;
    void* page;
    void* frame;
    struct hash_elem elem;
};
typedef struct frame_info frame_info;


void frame_init(void);
void* frame_get_frame(enum palloc_flags flag, void* user_page);
void frame_free_frame(void* frame);

#endif //VM_FRAME_H
