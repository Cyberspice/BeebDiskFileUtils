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

#include <stdlib.h>
#include <stdio.h>
#include "acornfs.h"
#include "acnfserr.h"
#include "debug.h"

/**
 * \brief Frees the memory of an Acorn directory
 *
 * This frees all the dynamic memory used by an Acorn directory
 * structure.
 *
 * \param acorn_dirp pointer to the directory
 * \return 0 on success or an error
 */
int acornfs_free_directory(ACORN_DIRECTORY * acorn_dirp) {
  ACORN_FILE * acorn_filep;

  if (acorn_dirp == NULL) {
    if (DEBUG_LEVEL(DEBUG_LEVEL_ERROR)) fprintf(stdout, "Invalid directory pointer!\n");
    return ACORNFS_ERROR_FAILED;
  }

  acorn_filep = acorn_dirp->files;
  for (int i = 0; i < acorn_dirp->num_of_files; i++) {
    free(acorn_filep->name);
    acorn_filep++;
  }

  free(acorn_dirp->name);
  free(acorn_dirp);

  return ACORNFS_ERROR_NONE;
}
