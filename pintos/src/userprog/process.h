#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

/* Ruihang Begin */
/* Used as the parameter of start_process().
 * "success" means that whether the new process is successfully loaded.
 */
struct process_start_info {
  char *file_name;            /* File name (or thread name?) of the thread. */
  bool success;               /* Whether start the process successfully. */
  semaphore start_sema;       /* Wait until start_process() finish. */
};
/* Ruihang End */

#endif /* userprog/process.h */
