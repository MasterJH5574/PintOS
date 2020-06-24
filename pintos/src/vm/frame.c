//
// Created by jinho on 6/24/2020.
//

#include "frame.h"
#include "threads/synch.h"

static struct lock mutex;

void frame_free_frame(void* frame) {
    lock_acquire(&mutex);
    lock_free(&mutex);
}