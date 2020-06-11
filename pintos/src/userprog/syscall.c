#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <string.h>
#include <user/syscall.h>
#include "devices/input.h"
#include "devices/shutdown.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/synch.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "pagedir.h"
#include "process.h"

static void syscall_handler (struct intr_frame *);

/* Ruihang Begin */

/* ------ Declarations of System Calls Begin ------ */
static void sys_halt(void);
void sys_exit(int status);
static pid_t sys_exec(const char *cmd_line);
static int sys_wait(pid_t pid);
static bool sys_create(const char *file, unsigned initialize_size);
static bool sys_remove(const char *file);
static int sys_open(const char* filename);
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


/* Get the struct file_descriptor of struct thread _thread. */
static struct file_descriptor *
get_file_descriptor(struct thread *_thread, int fd) {
  struct list_elem *_elem = list_begin(&(_thread->file_descriptors));
  while (_elem != list_end(&(_thread->file_descriptors))) {
    struct file_descriptor *_fd = list_entry(_elem,
                                    struct file_descriptor, elem);
    if (_fd->fd == fd)
      return _fd;
    _elem = list_next(_elem);
  }
  return NULL;
}

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
check_valid_syscall_args(void** syscall_args, int num) {
  for (int i = 0; i < num; i++)
    check_valid_user_addr(syscall_args[i], sizeof(uint32_t));
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
  for (const void *addr = user_buffer; addr < user_buffer + size; addr++)
    check_valid_user_addr(addr, sizeof(void));
}

/* Ruihang End */

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  /* Ruihang Begin */
  printf("addr of filesys_lock = %x\n", (&filesys_lock));
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

void
sys_exit(int status) {
  struct thread *cur_thread = thread_current();
  /*Jiaxin Begin*/
  //Free file resources
  while (!list_empty(&cur_thread->file_descriptors))
  {
    struct list_elem *e = list_pop_front(&cur_thread->file_descriptors);
    struct file_descriptor *fd = list_entry(e, struct file_descriptor, elem);
    file_close(fd->_file);
    free(fd);
  }
  /*Jiaxin End*/
  thread_current()->exit_code = status;

  /*Jiaxin Begin*/
  printf("%s: exit(%d)\n", thread_current()->name, status);
  /*Jiaxin End*/

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

static int
sys_open(const char* filename) {
  lock_acquire(&filesys_lock);
  struct file *_file = filesys_open(filename);
  lock_release(&filesys_lock);

  /* Return -1 if the file could not be opened. */
  if (_file == NULL)
    return -1;

  /* Malfunction until malloc() is implemented. */
  struct file_descriptor *fd = malloc(sizeof(struct file_descriptor));
  fd->fd = thread_current()->fd_num++;
  strlcpy(fd->file_name, filename, strlen(filename));
  fd->_file = _file;
  fd->owner_thread = thread_current();
  list_push_back(&(thread_current()->file_descriptors), &(fd->elem));

  return fd->fd;
}

static int
sys_filesize(int fd) {
  struct file_descriptor *_fd = get_file_descriptor(thread_current(), fd);
  /* Terminate if fd is not opened by the current thread. */
  if (_fd == NULL)
    sys_exit(-1);

  lock_acquire(&filesys_lock);
  int filesize = file_length(_fd->_file);
  lock_release(&filesys_lock);
  return filesize;
}

static int
sys_read(int fd, void *buffer, unsigned size) {
  /* If fd represent the stdout, terminate. */
  if (fd == STDOUT_FILENO)
    sys_exit(-1);

  unsigned res;
  uint8_t *ptr = buffer;
  if (fd == STDIN_FILENO) {
    /* Read from stdin using input_getc(). */
    res = size;
    while (size) {
      *ptr = input_getc();
      ptr++;
      size--;
    }
  } else {
    /* Read from file. */
    struct file_descriptor *_fd = get_file_descriptor(thread_current(), fd);
    /* Terminate if fd is not opened by the current thread. */
    if (_fd == NULL)
      sys_exit(-1);

    lock_acquire(&filesys_lock);
    res = file_read(_fd->_file, buffer, size);
    lock_release(&filesys_lock);
  }
  return (int)res;
}

static int
sys_write(int fd, const void *buffer, unsigned size) {
  /* If fd represent stdin, terminate. */
  if (fd == STDIN_FILENO)
    sys_exit(-1);

  unsigned res;
  if (fd == STDOUT_FILENO) {
    /* Output to stdout using putbuf(). */
    res = size;
    putbuf(buffer, size);
  } else {
    /* Write to file. */
    struct file_descriptor *_fd = get_file_descriptor(thread_current(), fd);
    /* Terminate if fd is not opened by the current thread. */
    if (_fd == NULL)
      sys_exit(-1);

    lock_acquire(&filesys_lock);
    res = file_write(_fd->_file, buffer, size);
    lock_release(&filesys_lock);
  }
  return (int)res;
}

static void
sys_seek(int fd, unsigned position) {
  struct file_descriptor *_fd = get_file_descriptor(thread_current(), fd);
  /* Terminate if fd is not opened by the current thread. */
  if (_fd == NULL)
    sys_exit(-1);

  lock_acquire(&filesys_lock);
  file_seek(_fd->_file, position);
  lock_release(&filesys_lock);
}

static unsigned
sys_tell(int fd) {
  struct file_descriptor *_fd = get_file_descriptor(thread_current(), fd);
  /* Terminate if fd is not opened by the current thread. */
  if (_fd == NULL)
    sys_exit(-1);

  lock_acquire(&filesys_lock);
  unsigned res = file_tell(_fd->_file);
  lock_release(&filesys_lock);
  return res;
}

static void
sys_close(int fd) {
  struct file_descriptor *_fd = get_file_descriptor(thread_current(), fd);
  /* Terminate if fd is not opened by the current thread. */
  if (_fd == NULL)
    sys_exit(-1);

  lock_acquire(&filesys_lock);
  file_close(_fd->_file);
  lock_release(&filesys_lock);

  list_remove(&(_fd->elem));
  free(_fd);
}
/* Ruihang End */
