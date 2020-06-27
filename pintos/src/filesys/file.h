#ifndef FILESYS_FILE_H
#define FILESYS_FILE_H

#include "filesys/off_t.h"
#include "kernel/list.h"

/* Ruihang Begin: file descriptor */
struct file_descriptor {
  int fd;
  char file_name[20];
  struct file *_file;                   /* _file == NULL if this is a dir. */
  struct dir* _dir;                     /* _dir == NULL if this is a file. */
                                        /* So at least one of them is NULL. */
  struct thread *owner_thread;
  struct list_elem elem;
};
/* Ruihang End */


struct inode;

/* Opening and closing files. */
struct file *file_open (struct inode *);
struct file *file_reopen (struct file *);
void file_close (struct file *);
struct inode *file_get_inode (struct file *);

/* Reading and writing. */
off_t file_read (struct file *, void *, off_t);
off_t file_read_at (struct file *, void *, off_t size, off_t start);
off_t file_write (struct file *, const void *, off_t);
off_t file_write_at (struct file *, const void *, off_t size, off_t start);

/* Preventing writes. */
void file_deny_write (struct file *);
void file_allow_write (struct file *);

/* File position. */
void file_seek (struct file *, off_t);
off_t file_tell (struct file *);
off_t file_length (struct file *);

#endif /* filesys/file.h */
