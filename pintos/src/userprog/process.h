#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

/* Used as the parameter of start_process().
 * "success" means that whether the new process is successfully loaded.
 * Todo: maybe a semaphore is needed.
 */
struct process_start_info {
  char *file_name;
  bool success;
};

#endif /* userprog/process.h */
