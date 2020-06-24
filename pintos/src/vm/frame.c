//
// Created by jinho on 6/24/2020.
//

#include "frame.h"

unsigned hash_frame(const hash_elem* e, void* aux){
    frame_info* info=hash_entry(e,frame_info,elem);
    return info->frame;
}

bool hash_frame_less(const hash_elem* a,const hash_elem* b, void * aux){
    return hash_frame(a,NULL)<hash_frame(b,NULL);
}


void __attribute__((optimize("-O0"))) frame_init () {
    hash_init(&frame_table,hash_frame,hash_frame_less,NULL);
}

void *frame_get_frame (void *user_page) {
    return NULL;
}
