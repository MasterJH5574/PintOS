#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "threads/thread.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);
bool check_valid_path_token(const char *token);
bool path_paser(const char *path, struct dir **dir, char **file_name, bool *is_dir);

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
    next_token = strtok(NULL, "/", &save_ptr);
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
#ifdef FILESYS
  //TODO: parse the name && use subfile_create to create a file
#endif
  block_sector_t inode_sector = 0;
  struct dir *dir = dir_open_root ();
  bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size)
                  && dir_add (dir, name, inode_sector));
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);

  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
#ifdef FILESYS
  //TODO: parse the name && 
  //  use subdir_loopup and subfile_lookup to find and open the file
#endif
  struct dir *dir = dir_open_root ();
  struct inode *inode = NULL;

  if (dir != NULL)
    dir_lookup (dir, name, &inode);
  dir_close (dir);

  return file_open (inode);
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
  struct dir *dir = dir_open_root ();
  bool success = dir != NULL && dir_remove (dir, name);
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
