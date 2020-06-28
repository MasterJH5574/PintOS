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

/* This is only used to check the validation of a token*/
bool
check_valid_path_token(const char *token)
{
  if (token == NULL)
    return false;
  /* Check the length of the token is within the MAX_DIR_LENGTH */
  for (int i = 0; i < MAX_DIR_LENGTH + 1; ++i)
  {
    if (token[i] == '\0')
      return true;
  }
  return false; //exceeds the max length
}

/*
* A parser to parse the path name
* Input: const char *path (the full path you get from the user, including the file name and path information)
* Output: struct dir *dir (dir parsed, it will be moved to the dir that the file in)
*         char* file_name (the real name of the file with no path information)
*         bool is_dir     (whether it's a pure dir (end with '/'))
* Return: bool            (true: the whole process worked, false: something wrong occured)
*/
bool
path_parser(const char *path, struct dir **dir, char **file_name, bool *is_dir)
{
  // First set the is_dir to false
  *is_dir = false;

  // Check the length of the path
  int length = strlen(path);
  if (length == 0)  //empty path
    return false;

  // Get a copy of the path for later use
  char *path_copy = malloc(length + 1);
  strlcpy(path_copy, path, length + 1);

  // Check whether a pure dir (end with '/'), and delete the end '/'.
  if (length > 0 && path_copy[length - 1] == '/')
  {
    *is_dir = true;
    --length;
    path_copy[length] = '\0';
  }

  // Check whether is the root path (the path "/")
  if (length == 0)
  {
    *dir = dir_open_root();
    (*file_name)[0] = '\0';
    free(path_copy);
    return true;
  }

  if (path_copy[0] == '/') // the root directory, file name like "/.../x"
    *dir = dir_open_root();
  else                    // normal directory, should use the directory saved in the thread info
    *dir = dir_reopen(thread_current()->current_dir);

  // Separate the token and move the dir
  char *token, *next_token, *save_ptr;
  for (token = strtok_r(path_copy, "/", &save_ptr); ; token = next_token)
  {
    // Check the validation of the token
    if (!check_valid_path_token(token))
    {
      dir_close(*dir);
      free(path_copy);
      return false;
    }
    ASSERT(token != NULL);
    next_token = strtok_r(NULL, "/", &save_ptr);
    if (next_token != NULL) // Token is not the file name
    {
      // Change dir to the new sub directory
      struct dir *tmp_dir = *dir;
      *dir = subdir_lookup(*dir, token);
      dir_close(tmp_dir);
      if (*dir == NULL)
      {
        free(path_copy);
        return false;
      }
    } else  // Token is the file name, break
      break;
  }
  // Token is the file name and save it into file_name
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
  char file_name_buffer[15];
  char* file_name=file_name_buffer;
  struct dir* dir;
  bool is_dir;
  if(path_parser(name, &dir, &file_name, &is_dir)){
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

  bool parse_success = path_parser(name, &dir, &file_name, &is_dir);
  if (!parse_success || is_dir) {
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
  struct dir* dir;
  char file_name_buffer[15];
  char* file_name=file_name_buffer;
  bool is_dir;

  bool parse_success = path_parser(name, &dir, &file_name, &is_dir);
  if (!parse_success)
    return false;

  bool remove_success = false;
  ASSERT(dir != NULL)
  if (is_dir) {
    /* NAME designates a pure directory, just remove it unless it is
     * the root directory.
     */
    if (dir_is_root_dir(dir))
      remove_success = false;
    else
      remove_success = subdir_delete(dir, file_name);
  } else {
    /* Whether FILE_NAME designates a file or a directory, just call "remove".*/
    remove_success = subfile_remove(dir, file_name)
                     || subdir_delete(dir,file_name);
  }
  dir_close(dir);
  return remove_success;
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

/* Open the directory designated by NAME. Return NULL if not found. */
struct dir* filesys_opendir(const char*name){
  struct dir* dir;
  char file_name_buffer[15];
  char* file_name=file_name_buffer;
  bool is_dir;
  bool parse_success = path_parser(name, &dir, &file_name, &is_dir);
  if (!parse_success)
    return NULL;

  if (is_dir) {
    /* If NAME designates the root directory, just return dir. */
    if (file_name[0] == '\0')
      return dir;
    else {
      struct dir *res = subdir_lookup(dir, file_name);
      dir_close(dir);
      return res;
    }
  } else {
    /* If NAME does not designate a pure directory, try to look subdirectory
     * FILE_NAME in DIR.
     * If the subdirectory FILE_NAME does not exist, RES will be NULL.
     */
    struct dir *res = subdir_lookup(dir, file_name);
    dir_close(dir);
    return res;
  }
}
