#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <threads/vaddr.h>
#include <lib/user/syscall.h>
#include <devices/shutdown.h>
#include <filesys/filesys.h>
#include <threads/synch.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "pagedir.h"
#include "process.h"

static void syscall_handler (struct intr_frame *);

/* Ruihang Begin */

/* ------ Declarations of System Calls Begin ------ */
static void sys_halt(void);
static void sys_exit(int status);
static pid_t sys_exec(const char *cmd_line);
static int sys_wait(pid_t pid);
static bool sys_create(const char *file, unsigned initialize_size);
static bool sys_remove(const char *file);
static int sys_open(const char* file);
static int sys_filesize(int fd);
static int sys_read(int fd, void *buffer, unsigned size);
static int sys_write(int fd, const void *buffer, unsigned size);
static void sys_seek(int fd, unsigned position);
static unsigned sys_tell(int fd);
static void sys_close(int fd);
/* ------ Declarations of System Calls End ------ */


/* File system lock to ensure that there is at most one system call related to
 * file system at one time.
 */
struct lock filesys_lock;


/* Check whether the address given by the user program which invoked a system
 * call is valid:
 *   1. not null pointer;
 *   2. not a pointer to kernel virtual address space (above PHYS_BASE);
 *   3. not a pointer to unmapped virtual memory.
 */
static void
check_valid_user_addr(const void *user_addr, uint32_t size) {
  for (const void *addr = user_addr; addr < user_addr + size; addr++) {
    if (!addr
     || !is_user_vaddr(addr)
     || pagedir_get_page(thread_current()->pagedir, addr) == NULL) {
      sys_exit(-1);
      return;
    }
  }
}

/* Return the system call number. */
static uint32_t
get_syscall_number(struct intr_frame *f) {
  check_valid_user_addr(f->esp, sizeof(uint32_t));
  return *((uint32_t *)(f->esp));
}

/* Check whether the parameters of a system call are all valid. */
static void
check_valid_syscall_args(void* syscall_args, int num) {
  for (int i = 0; i < num; i++)
    check_valid_user_addr(syscall_args + i, sizeof(uint32_t));
}

/* Check whether the address of the user string is valid. */
static void
check_valid_user_string(const void *user_string) {
  check_valid_user_addr(user_string, sizeof(char));

  int len = 0;
  while (*((char *)user_string) != '\0') {
    if (len == 0xfff) {
      // The length of user_string is more than 4KB - a single page. Reject.
      sys_exit(-1);
    }
    len++;
    user_string++;
    check_valid_user_addr(user_string, sizeof(char));
  }
}

/* Check whether the address of the user buffer is valid. */
static void
check_valid_user_buffer(const void *user_buffer, unsigned size) {
  if (size == 0xfff) {
    // The length of user_buffer is more than 4KB - a single page. Reject.
    sys_exit(-1);
  }

  for (const void *addr = user_buffer; addr < user_buffer + size; addr++)
    check_valid_user_addr(addr, sizeof(void));
}

/* Ruihang End */

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  /* Ruihang Begin */
  lock_init(&filesys_lock);
  /* Ruihang End */
}


/* Ruihang Begin */
static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  uint32_t syscall_number = get_syscall_number(f);
  void* syscall_args[3] = {f->esp + 4, f->esp + 8, f->esp + 12};

  switch (syscall_number) {
    case SYS_HALT:
      sys_halt();
      break;
    case SYS_EXIT:
      check_valid_syscall_args(syscall_args, 1);
      sys_exit(*((int*)syscall_args[0]));
      break;
    case SYS_EXEC:
      check_valid_syscall_args(syscall_args, 1);
      check_valid_user_string(*((const char **)syscall_args[0]));
      f->eax = sys_exec(*((const char **)syscall_args[0]));
      break;
    case SYS_WAIT:
      check_valid_syscall_args(syscall_args, 1);
      f->eax = sys_wait(*((pid_t *)syscall_args[0]));
      break;
    case SYS_CREATE:
      check_valid_syscall_args(syscall_args, 2);
      check_valid_user_string(*((const char **)syscall_args[0]));
      f->eax = sys_create(*((const char **)syscall_args[0]),
          *((unsigned *)syscall_args[1]));
      break;
    case SYS_REMOVE:
      check_valid_syscall_args(syscall_args, 1);
      check_valid_user_string(*((const char **)syscall_args[0]));
      f->eax = sys_remove(*((const char **)syscall_args[0]));
      break;
    case SYS_OPEN:
      check_valid_syscall_args(syscall_args, 1);
      check_valid_user_string(*((const char **)syscall_args[0]));
      f->eax = sys_open(*((const char **)syscall_args[0]));
      break;
    case SYS_FILESIZE:
      check_valid_syscall_args(syscall_args, 1);
      f->eax = sys_filesize(*((int *)syscall_args[0]));
      break;
    case SYS_READ:
      check_valid_syscall_args(syscall_args, 3);
      check_valid_user_buffer(*((const void **)syscall_args[1]),
          *((unsigned *)syscall_args[2]));
      f->eax = sys_read(*((int *)syscall_args[0]),
          *((void **)syscall_args[1]),
          *((unsigned *)syscall_args[2]));
      break;
    case SYS_WRITE:
      check_valid_syscall_args(syscall_args, 3);
      check_valid_user_buffer(*((const void **)syscall_args[1]),
                              *((unsigned *)syscall_args[2]));
      f->eax = sys_write(*((int *)syscall_args[0]),
                        *((const void **)syscall_args[1]),
                        *((unsigned *)syscall_args[2]));
      break;
    case SYS_SEEK:
      check_valid_syscall_args(syscall_args, 2);
      sys_seek(*((int *)syscall_args[0]),
          *((unsigned *)syscall_args[1]));
      break;
    case SYS_TELL:
      check_valid_syscall_args(syscall_args, 1);
      f->eax = sys_tell(*((int *)syscall_args[0]));
      break;
    case SYS_CLOSE:
      check_valid_syscall_args(syscall_args, 1);
      sys_close(*((int *)syscall_args[0]));
      break;
    default:
      // Todo: implement more in project 3 and 4
      break;
  }
}

static void
sys_halt() {
  shutdown_power_off();
}

static void
sys_exit(int status) {
  struct thread *cur_thread = thread_current();
  // Todo: free resources
  thread_current()->exit_code = status;
  thread_exit();
}

static pid_t
sys_exec(const char *cmd_line) {
  return process_execute(cmd_line);
}
/* Ruihang End */

static int
sys_wait(pid_t pid) {
  /*Jiaxin Begin*/
  return process_wait(pid);
  /*Jiaxin end*/
}

/* Ruihang Begin */
static bool
sys_create(const char *file, unsigned initialize_size) {
  lock_acquire(&filesys_lock);
  bool res = filesys_create(file, initialize_size);
  lock_release(&filesys_lock);
  return res;
}

static bool
sys_remove(const char *file) {
  lock_acquire(&filesys_lock);
  int res = filesys_remove(file);
  lock_release(&filesys_lock);
  return res;
}
/* Ruihang End */

static int
sys_open(const char* file) {
  // Todo
}

static int
sys_filesize(int fd) {
  // Todo
}

static int
sys_read(int fd, void *buffer, unsigned size) {
  // Todo
}

static int
sys_write(int fd, const void *buffer, unsigned size) {
  // Todo
}

static void
sys_seek(int fd, unsigned position) {
  // Todo
}

static unsigned
sys_tell(int fd) {
  // Todo
}

static void
sys_close(int fd) {
  // Todo
}
