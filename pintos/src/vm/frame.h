//
// Created by jinho on 6/24/2020.
//
#include "threads/thread.h"
#include "hash.h"
#ifndef VM_FRAME_H
#define VM_FRAME_H

struct frame_info{
    thread* thread_hold;
    void* page;
    void* frame;
};
hash frame_table;

void frame_init();
void* frame_get_frame(void* user_page);
void frame_free_frame(void* frame);

#endif //VM_FRAME_H
