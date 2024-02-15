/*
MIT License

Copyright (c) 2022 Cyberspice cyberspice@cyberspice.org.uk

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef __ACORNFS_H
#define __ACORNFS_H

#include "defs.h"

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
  ACORN_FILE files[1];
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

