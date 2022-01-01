#ifndef __ACORNFS_H
#define __ACORNFS_H

#include <stdint.h>

typedef enum {
  LOCKED = 0x80
} ACORN_FILE_ATTRIBS;

typedef struct {
  char * name;
  uint32_t load_address;
  uint32_t exec_address;
  uint32_t length;
  ACORN_FILE_ATTRIBS attributes;
  uint32_t start_sector;
} ACORN_FILE;

typedef struct _tag_ACORN_DIRECTORY {
  struct _tag_ACORN_DIRECTORY * parent;
  char * name;
  uint8_t options;
  int num_of_files;
  ACORN_FILE files[];
} ACORN_DIRECTORY;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Frees the memory of an Acorn directory
 *
 * This frees all the dynamic memory used by an Acorn directory
 * structure.
 *
 * \param acorn_dirp pointer to the directory
 * \return 0 on success or an error
 */
int acornfs_free_directory(ACORN_DIRECTORY * acorn_dirp);

#ifdef __cplusplus
}
#endif

#endif /* __ACORNFS_H */

