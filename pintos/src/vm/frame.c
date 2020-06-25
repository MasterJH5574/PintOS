//
// Created by jinho on 6/24/2020.
//

#include "frame.h"
static struct lock mutex;
static hash frame_table;

unsigned hash_frame(const hash_elem* e, void* aux){
    frame_info* info=hash_entry(e,frame_info,elem);
    return hash_int((int) info->frame);
}

bool hash_frame_less(const hash_elem* a,const hash_elem* b, void * aux){
    return hash_frame(a,NULL)<hash_frame(b,NULL);
}


void __attribute__((optimize("-O0"))) frame_init () {
    hash_init(&frame_table,hash_frame,hash_frame_less,NULL);
    lock_init(&mutex);
}

void *frame_get_frame (enum palloc_flags flag,void *user_page) {
    lock_acquire(&mutex);
    void* frame=palloc_get_page(flag|PAL_USER);
    
    if (frame == NULL) {
        PANIC("there is no fucking frame.");
    }
    
    frame_info* new_info=(frame_info*)malloc(sizeof(frame_info));
    new_info->frame=frame;
    new_info->page=user_page;
    new_info->thread_hold=thread_current();
    hash_insert(&frame_table,&new_info->elem);
    lock_release(&mutex);
    return frame;
}

void frame_free_frame(void* frame) {
    lock_acquire(&mutex);

    struct frame_info tmp, *info;
    struct hash_elem * elem;
    tmp.frame = frame;
    elem = hash_find(&frame_table, &tmp.elem);
    if (elem == NULL) PANIC("invalid frame to free");
    info = hash_entry(elem, struct frame_info, elem);
    hash_delete(&frame_table, &info->elem);
    free(info);
    palloc_free_page(frame);
    lock_release(&mutex);
}