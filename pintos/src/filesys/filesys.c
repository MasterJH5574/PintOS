#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "threads/thread.h"
#include "threads/malloc.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);
bool check_valid_path_token(const char *token);

/* Jiaxin Begin */

bool
check_valid_path_token(const char *token)
{
  if (token == NULL)
    return false;
  for (int i = 0; i < MAX_DIR_LENGTH + 1; ++i)
  {
    if (token[i] == '\0')
      return true;
  }
  return false; //exceeds the max length
}

/*
* A parser to parse the path name
*/
bool
path_paser(const char *path, struct dir **dir, char **file_name, bool *is_dir)
{
  *is_dir = false;
  int length = strlen(path);
  if (length == 0)
    return false;

  //get a copy of path
  char *path_copy = malloc(length + 1);
  strlcpy(path_copy, path, length + 1);

  if (length > 0 && path_copy[length - 1] == '/')
  {
    *is_dir = true;
    --length;
    path_copy[length] = '\0';
  }

  if (length == 0)  //is root path
  {
    free(path_copy);
    return false;
  }

  if (path_copy[0] == '/')
    *dir = dir_open_root();
  else
    *dir = dir_reopen(thread_current()->current_dir);

  char *token, *next_token, *save_ptr;
  for (token = strtok_r(path_copy, "/", &save_ptr); ; token = next_token)
  {
    if (!check_valid_path_token(token))
    {
      dir_close(*dir);
      free(path_copy);
      return false;
    }
    ASSERT(token != NULL);
    next_token = strtok_r(NULL, "/", &save_ptr);
    if (next_token != NULL)
    {
      struct dir *tmp_dir = *dir;
      *dir = subdir_lookup(*dir, token);
      dir_close(tmp_dir);
      if (*dir == NULL)
      {
        free(path_copy);
        return false;
      }
    } else  //token is the file name
      break;
  }
  strlcpy(*file_name, token, MAX_DIR_LENGTH + 1);
  free(path_copy);
  return true;
}

/* Jiaxin End */

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  cache_flush();
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size) 
{

  block_sector_t inode_sector = 0;
  char file_name_buffer[15];
  char* file_name=file_name_buffer;
  struct dir* dir;
  bool is_dir;
  if(path_paser(name,&dir,&file_name,&is_dir)){
    if (is_dir) {
      dir_close(dir);
      return false;
    }
    bool success=subfile_create(dir,file_name,initial_size);
    dir_close(dir);
    return success;
  }
  return false;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails.
   can not open a directory!!!
   */
struct file *
filesys_open (const char *name)
{
  
  struct dir* dir;
  char file_name_buffer[15];
  char* file_name=file_name_buffer;
  bool is_dir;
  path_paser(name,&dir,&file_name,&is_dir);
  if (is_dir) {
    return NULL;
  }
  struct file* file=subfile_lookup(dir,file_name);
  dir_close (dir);

  return file;
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
#ifdef FILESYS
  //TODO: parse the name && 
  //  use subdir_remove and subfile_remove to remove a file
#endif
  struct dir* dir;
  char file_name_buffer[15];
  char* file_name=file_name_buffer;
  bool is_dir;
  path_paser(name,&dir,&file_name,&is_dir);
  bool success = dir != NULL && (subfile_remove(dir, file_name) || subdir_delete
                                     (dir,file_name));
  dir_close (dir); 

  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}
struct dir* filesys_opendir(const char*name){
  struct dir* dir;
  char file_name_buffer[15];
  char* file_name=file_name_buffer;
  bool is_dir;
  path_paser(name,&dir,&file_name,&is_dir);
  struct dir* open_dir=subdir_lookup(dir,file_name);
  dir_close (dir);
  
  return open_dir;
}
