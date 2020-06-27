#include "filesys/directory.h"
#include <stdio.h>
#include <string.h>
#include <list.h>
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "threads/malloc.h"
#include "filesys/free-map.h"

/* A directory. */
struct dir 
  {
    struct inode *inode;                /* Backing store. */
    off_t pos;                          /* Current position. */
  };

/* A single directory entry. */
struct dir_entry 
  {
    block_sector_t inode_sector;        /* Sector number of header. */
    char name[NAME_MAX + 1];            /* Null terminated file name. */
    bool in_use;                        /* In use or free? */
  };

/* Creates a directory with space for ENTRY_CNT entries in the
   given SECTOR.  Returns true if successful, false on failure. */
bool
dir_create (block_sector_t sector, size_t entry_cnt)
{
  bool success = inode_create (sector, (DIR_BASE_ENTRIES_NUM + entry_cnt)
                               * sizeof (struct dir_entry));
  if (success) {
    struct inode *inode = inode_open(sector);
    ASSERT(inode != NULL)
    inode_set_dir(inode);

    struct dir *dir = dir_open(inode);
    ASSERT(dir != NULL)
    bool add1 = dir_add(dir, ".", sector);
    bool add2 = dir_add(dir, "..", sector);
    ASSERT(add1 && add2)

    inode_close(inode);
  }
  return success;
}

/* Opens and returns the directory for the given INODE, of which
   it takes ownership.  Returns a null pointer on failure. */
struct dir *
dir_open (struct inode *inode) 
{
  struct dir *dir = calloc (1, sizeof *dir);
  if (inode != NULL && dir != NULL)
    {
      dir->inode = inode;
      dir->pos = DIR_BASE_ENTRIES_NUM * sizeof(struct dir_entry);
      return dir;
    }
  else
    {
      inode_close (inode);
      free (dir);
      return NULL; 
    }
}

/* Opens the root directory and returns a directory for it.
   Return true if successful, false on failure. */
struct dir *
dir_open_root (void)
{
  return dir_open (inode_open (ROOT_DIR_SECTOR));
}

/* Opens and returns a new directory for the same inode as DIR.
   Returns a null pointer on failure. */
struct dir *
dir_reopen (struct dir *dir) 
{
  return dir_open (inode_reopen (dir->inode));
}

/* Destroys DIR and frees associated resources. */
void
dir_close (struct dir *dir) 
{
  if (dir != NULL)
    {
      inode_close (dir->inode);
      free (dir);
    }
}

/* Returns the inode encapsulated by DIR. */
struct inode *
dir_get_inode (struct dir *dir) 
{
  return dir->inode;
}

/* Searches DIR for a file with the given NAME.
   If successful, returns true, sets *EP to the directory entry
   if EP is non-null, and sets *OFSP to the byte offset of the
   directory entry if OFSP is non-null.
   otherwise, returns false and ignores EP and OFSP. */
static bool
lookup (const struct dir *dir, const char *name,
        struct dir_entry *ep, off_t *ofsp) 
{
  struct dir_entry e;
  size_t ofs;
  
  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e) 
    if (e.in_use && !strcmp (name, e.name)) 
      {
        if (ep != NULL)
          *ep = e;
        if (ofsp != NULL)
          *ofsp = ofs;
        return true;
      }
  return false;
}

/* Searches DIR for a file with the given NAME
   and returns true if one exists, false otherwise.
   On success, sets *INODE to an inode for the file, otherwise to
   a null pointer.  The caller must close *INODE. */
bool
dir_lookup (const struct dir *dir, const char *name,
            struct inode **inode) 
{
  struct dir_entry e;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  if (lookup (dir, name, &e, NULL))
    *inode = inode_open (e.inode_sector);
  else
    *inode = NULL;

  return *inode != NULL;
}

/* Adds a file named NAME to DIR, which must not already contain a
   file by that name.  The file's inode is in sector
   INODE_SECTOR.
   Returns true if successful, false on failure.
   Fails if NAME is invalid (i.e. too long) or a disk or memory
   error occurs. */
bool
dir_add (struct dir *dir, const char *name, block_sector_t inode_sector)
{
  struct dir_entry e;
  off_t ofs;
  bool success = false;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  /* Check NAME for validity. */
  if (*name == '\0' || strlen (name) > NAME_MAX)
    return false;

  /* Check that NAME is not in use. */
  if (lookup (dir, name, NULL, NULL))
    goto done;

  /* Set OFS to offset of free slot.
     If there are no free slots, then it will be set to the
     current end-of-file.
     
     inode_read_at() will only return a short read at end of file.
     Otherwise, we'd need to verify that we didn't get a short
     read due to something intermittent such as low memory. */
  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e) 
    if (!e.in_use)
      break;

  /* Write slot. */
  e.in_use = true;
  strlcpy (e.name, name, sizeof e.name);
  e.inode_sector = inode_sector;
  success = inode_write_at (dir->inode, &e, sizeof e, ofs) == sizeof e;

 done:
  return success;
}

/* Removes any entry for NAME in DIR.
   Returns true if successful, false on failure,
   which occurs only if there is no file with the given NAME. */
bool
dir_remove (struct dir *dir, const char *name) 
{
  struct dir_entry e;
  struct inode *inode = NULL;
  bool success = false;
  off_t ofs;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  /* Find directory entry. */
  if (!lookup (dir, name, &e, &ofs))
    goto done;

  /* Open inode. */
  inode = inode_open (e.inode_sector);
  if (inode == NULL)
    goto done;

  /* Erase directory entry. */
  e.in_use = false;
  if (inode_write_at (dir->inode, &e, sizeof e, ofs) != sizeof e) 
    goto done;

  /* Remove inode. */
  inode_remove (inode);
  success = true;

 done:
  inode_close (inode);
  return success;
}

/* Reads the next directory entry in DIR and stores the name in
   NAME.  Returns true if successful, false if the directory
   contains no more entries. */
bool
dir_readdir (struct dir *dir, char name[NAME_MAX + 1])
{
  struct dir_entry e;

  while (inode_read_at (dir->inode, &e, sizeof e, dir->pos) == sizeof e) 
    {
      dir->pos += sizeof e;
      if (e.in_use)
        {
          strlcpy (name, e.name, NAME_MAX + 1);
          return true;
        } 
    }
  return false;
}

/* ZYHowell: subdir*/
/* Find the subdirectory designated by NAME in directory CUR.
 * Find it, open it, and then return it.
 * If there is no subdirectory named NAME, return NULL.
 */
struct dir*
subdir_lookup(struct dir *cur, char *name)
{
  if (cur == NULL || name == NULL || strlen(name) == 0) return NULL; //or ASSERT?
  struct inode *node = NULL;
  if (!dir_lookup(cur, name, &node)) return NULL;
  if (!inode_isdir(node)) {
    inode_close(node);
    return NULL;
  }
  return dir_open(node);
}

/* Create a subdirectory named NAME in directory CUR.
 * Return whether the creation is successful.
 */
bool subdir_create(struct dir *cur, char *name) {
  if (cur == NULL || name == NULL || strlen(name) == 0) return false; //or ASSERT?
  block_sector_t sector = (block_sector_t) -1;
  bool success = free_map_allocate(1, &sector)
              && dir_create(sector, 0)
              && dir_add(cur, name, sector);
  if (!success && sector != ((block_sector_t) -1)) free_map_release(sector,1);
  return success; 
}

/* Delete the subdirectory designated by NAME in the directory CUR.
 * Return whether the deletion is successful.
 * Return false, if
 *  - there is no subdirectory named NAME,
 *  - or if the inode designated by NAME is not a subdirectory,
 *  - or if the subdirectory NAME is not empty.
 */
bool subdir_delete(struct dir *cur, char *name) {
  if (cur == NULL || name == NULL || strlen(name) == 0) return false;

  /* Lookup the subdirectory designated by NAME. Return false if not found. */
  struct dir *dir_to_be_removed = subdir_lookup(cur, name);
  if (dir_to_be_removed == NULL)
    return false;

  struct inode *node = dir_to_be_removed->inode;
  ASSERT(inode_isdir(node))

  /* Return false if the directory is opened by others. */
  if (inode_get_opencnt(node) > 1)
    return false;

  char* buffer = malloc(NAME_MAX + 1);
  if (dir_readdir(dir_to_be_removed, buffer)) { // remove empty directory only.
    dir_close(cur);
    dir_close(dir_to_be_removed);
    free(buffer);
    return false;
  }
  free(buffer);
  dir_close(dir_to_be_removed);
  return dir_remove(cur, name);
}

/*Jiaxin Begin*/
/* Create a file with name FILE_NAME and initial size INITIAL_SIZE in
 * the directory DIR.
 */
bool
subfile_create(struct dir* dir, char* file_name, off_t initial_size)
{
  ASSERT(file_name != NULL);
  if (strlen(file_name) == 0)
    return false;
  if (dir == NULL)
    return false;
  block_sector_t block_sector = (block_sector_t) -1;
  bool success = free_map_allocate(1, &block_sector)
              && inode_create(block_sector, initial_size)
              && dir_add(dir, file_name, block_sector);
  if (!success && block_sector != (block_sector_t) -1)
    free_map_release(block_sector, 1);
  return success;
}

/* Remove the file named FILE_NAME in the directory DIR.
 * Return false if there is no file FILE_NAME or the found inode is a directory.
 */
bool
subfile_remove(struct dir* dir, char* file_name)
{
  ASSERT(dir != NULL);
  ASSERT(file_name != NULL);
  if (strlen(file_name) == 0)
    return false;
  struct inode* inode = NULL;
  bool ret = dir_lookup(dir, file_name, &inode);
  if (!ret || inode == NULL)
    return false;
  if (inode_isdir(inode))
  {
    inode_close(inode);
    return false;
  }
  inode_close(inode);
  return dir_remove(dir, file_name);
}

/* Find the file named FILE_NAME in the directory DIR.
 * Return NULL if there is no file FILE_NAME or the found inode is a directory.
 */
struct file *
subfile_lookup(struct dir* dir, char* file_name)
{
  ASSERT(dir != NULL);
  ASSERT(file_name != NULL);
  if (strlen(file_name) == 0)
    return false;
  struct inode* inode = NULL;
  bool ret = dir_lookup(dir, file_name, &inode);
  if (!ret || inode == NULL)
    return false;
  if (inode_isdir(inode))
  {
    inode_close(inode);
    return false;
  }
  struct file* file = file_open(inode);
  return file;
}

bool
dir_is_root_dir(struct dir *dir) {
  return inode_is_root(dir->inode);
}
/*Jiaxin End*/
