#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

extern struct lock filesys_lock;

void syscall_init (void);

/* Ruihang Begin: sys_exit may be invoked by other function. */
void sys_exit(int);
/* Ruihang End */

#endif /* userprog/syscall.h */
