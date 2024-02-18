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

/* Defines the interface for reading and writing a disk image file */

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "defs.h"
#include "diskimage.h"


#define EXTENT_UNKNOWN UINT32_MAX

/* File handle for accessing this object */
struct diskimagedata_s {
    FILE *fh;
    uint32_t offset;
    uint32_t extent;
};


/* Close a disk image */
static void diskimage_closefile(diskimage_t *di)
{
    if (di == NULL)
        return;
    if (di->image->fh == NULL)
        return; /* Shouldn't happen, but we'll be safer */

    fclose(di->image->fh);
    di->image->fh = NULL;
}


/* Get the size of the image, in bytes */
static uint32_t diskimage_get_extent(diskimage_t *di)
{
    uint32_t extent;

    if (di == NULL)
        return 0;

    extent = di->image->extent;
    if (extent == EXTENT_UNKNOWN)
    {
        int ret;
        ret = fseek(di->image->fh, 0, SEEK_END);
        if (ret == -1)
            /* Cannot seek, so return 0 - which would be impossible to use */
            return 0;

        extent = ftell(di->image->fh);

        di->image->offset = extent;
        di->image->extent = extent;
    }

    return extent;
}

/* Read bytes from a disk image; may be non-sector aligned */
static int diskimage_read(diskimage_t *di, void *dest, size_t size, uint32_t offset)
{
    int read;
    if (di == NULL)
        return 0; /* Read no bytes if the handle invalid */

    if (di->image->offset != offset)
    {
        int ret;
        ret = fseek(di->image->fh, offset, SEEK_SET);
        if (ret == -1) {
            /* Cannot seek, so return failure */
            return -1;
        }
        di->image->offset = offset;
    }

    read = fread(dest, sizeof(uint8_t), size, di->image->fh);
    if (read < 0) {
        read = -1;
    } else {
        di->image->offset += read;
    }
    return read;
}


/* Write bytes to a disk image; may be non-sector aligned */
static int diskimage_write(diskimage_t *di, void *src, size_t size, uint32_t offset)
{
    int wrote;
    if (di == NULL)
        return 0; /* Read no bytes if the handle invalid */

    if (di->image->offset != offset)
    {
        int ret;
        ret = fseek(di->image->fh, offset, SEEK_SET);
        if (ret == -1) {
            /* Cannot seek, so return failure */
            return -1;
        }
        di->image->offset = offset;
    }

    wrote = fwrite(src, sizeof(uint8_t), size, di->image->fh);
    if (wrote < 0) {
        wrote = -1;
    } else {
        di->image->offset += wrote;
    }
    return wrote;
}



/* Open a disk image as a file.
 *
 * Returns a DISKIMAGE_ERROR_* value, and updates the pointer to the image handle.
 */
int diskimage_openfile(const char *filename, int mode, diskimage_t **dip)
{
    diskimage_t *di;
    char *filemode = "rb";
    FILE *fh;

    switch (mode)
    {
        case DISKIMAGE_MODE_READ:
            filemode = "rb";
            break;

        case DISKIMAGE_MODE_CREATE:
            filemode = "w+b";
            break;

        case DISKIMAGE_MODE_UPDATE:
            filemode = "r+b";
            break;

        default:
            /* If they requested anything else, we cannot access the file */
            return DISKIMAGE_ERROR_FAILED;
    }

    di = calloc(1, sizeof(*di));
    if (di == NULL)
    {
        return DISKIMAGE_ERROR_NO_MEMORY;
    }
    di->image = calloc(1, sizeof(*di->image));
    if (di->image == NULL)
    {
        free(di);
        return DISKIMAGE_ERROR_NO_MEMORY;
    }

    fh = fopen(filename, filemode);
    if (fh == NULL) {
        int err = DISKIMAGE_ERROR_FAILED;
#ifndef __riscos
        if (errno == ENOENT) {
            err = DISKIMAGE_ERROR_NOT_FOUND;
        }
        if (mode == DISKIMAGE_MODE_CREATE &&
            (errno == EISDIR ||
             errno == EACCES))
        {
            err = DISKIMAGE_ERROR_CANNOT_CREATE;
        }
#endif
        free(di->image);
        free(di);
        return err;
    }

    di->image->fh = fh;
    di->image->offset = 0;
    di->image->extent = EXTENT_UNKNOWN;

    /* Assign the methods */
    di->close = diskimage_closefile;
    di->get_extent = diskimage_get_extent;
    di->read = diskimage_read;
    di->write = diskimage_write;

    *dip = di;
    return DISKIMAGE_ERROR_NONE;
}
