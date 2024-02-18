/*
MIT License

Copyright (c) 2024 Gerph <gerph@gerph.org>

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

/* Defines the interface for reading and writing a disk image */

#ifndef DISKIMAGE_H
#define DISKIMAGE_H

#include "defs.h"

#define DISKIMAGE_ERROR_NONE                0
#define DISKIMAGE_ERROR_FAILED              0x20000 /* Non-specific failure */
#define DISKIMAGE_ERROR_NOT_FOUND           0x20001
#define DISKIMAGE_ERROR_CANNOT_CREATE       0x20002
#define DISKIMAGE_ERROR_NO_MEMORY           0x20003

#define DISKIMAGE_MODE_READ   (1)
#define DISKIMAGE_MODE_CREATE (2)
#define DISKIMAGE_MODE_UPDATE (3)

/* Structure that defines that disk image handle */
typedef struct diskimagedata_s diskimagedata_t;


typedef struct diskimage_s diskimage_t;

struct diskimage_s {
    /* Private data for this disk */
    diskimagedata_t *image;

    /* Close a disk image */
    void (*close)(diskimage_t *di);

    /* Get the size of the image, in bytes */
    uint32_t (*get_extent)(diskimage_t *di);

    /* Read bytes from a disk image; may be non-sector aligned */
    int (*read)(diskimage_t *di, void *dest, size_t size, uint32_t offset);

    /* Write bytes to a disk image; may be non-sector aligned */
    int (*write)(diskimage_t *di, void *src, size_t size, uint32_t offset);
};

#endif
