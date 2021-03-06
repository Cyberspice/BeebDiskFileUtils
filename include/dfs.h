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

#ifndef __DFS_H
#define __DFS_H

#include <stdint.h>
#include "acornfs.h"
#include "dfserr.h"

#define DFS_SECTOR_SIZE 256
#define DFS_SECTORS_PER_TRACK 10
#define DFS_MAX_FILES  31

typedef struct {
  char diskname_0[8];
} DFS_DISK_NAME_0;

typedef struct {
  char filename[7];
  char directory;
} DFS_FILE_NAME;

typedef struct {
  char diskname_1[4];
  uint8_t cycle_number;
  uint8_t num_of_files;
  uint8_t num_of_sectors_high;
  uint8_t num_of_sectors_low;
} DFS_DISK_NAME_1;

typedef struct {
  uint8_t load_address_low;
  uint8_t load_address_high;
  uint8_t exec_address_low;
  uint8_t exec_address_high;
  uint8_t length_low;
  uint8_t length_high;
  uint8_t start_sector_high;
  uint8_t start_sector_low;
} DFS_FILE_PARAMS;

typedef struct {
  DFS_DISK_NAME_0 disk_name_0;
  DFS_FILE_NAME file_names[DFS_MAX_FILES];
} DFS_SECTOR_0;

typedef struct {
  DFS_DISK_NAME_1 disk_name_1;
  DFS_FILE_PARAMS file_params[DFS_MAX_FILES];
} DFS_SECTOR_1;

#define DFS_NUM_OF_FILES_MASK  0xf8
#define DFS_NUM_OF_FILES_SHIFT 3

#define DFS_NUM_OF_SECTORS_LOW_MASK 0x03
#define DFS_40_TRACK_NUM_OF_SECTORS 400
#define DFS_80_TRACK_NUM_OF_SECTORS 800

#define DFS_BOOT_OPTIONS_MASK 0x30
#define DFS_BOOT_OPTION_NONE 0
#define DFS_BOOT_OPTION_LOAD 1
#define DFS_BOOT_OPTION_RUN  2
#define DFS_BOOT_OPTION_EXEC 3

#define DFS_MAX_DISK_NAME_LEN 12
#define DFS_MAX_DIR_NAME_LEN  1
#define DFS_DIR_NAME_MASK     0x7f
#define DFS_MAX_FILE_NAME_LEN 7
#define DFS_LOCK_BIT          0x80

#define DFS_START_SECTOR_HIGH_MASK      0x03
#define DFS_START_SECTOR_HIGH_SHIFT     0
#define DFS_LOAD_ADDRESS_BIT_17_18_MASK 0x0c
#define DFS_LOAD_ADDRESS_SHIFT          2
#define DFS_FILE_LENGTH_BIT_17_18_MASK  0x30
#define DFS_FILE_LENGTH_SHIFT           4
#define DFS_EXEC_ADDRESS_BIT_17_18_MASK 0xc0
#define DFS_EXEC_ADDRESS_SHIFT          6

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief reads the catalogue from a DFS disk
 *
 * This function reads the catalogue from a DFS disk. It assumes the file
 * pointer is set to the start of the disk image file.
 *
 * \param diskfile the disk image file reference
 * \param acorn_dirpp pointer in which to return the acorn directory
 * \return 0 on success or an error
 */
int dfs_read_catalogue(FILE * diskfile, ACORN_DIRECTORY ** acorn_dirpp);

/**
 * \brief Creates an empty DFS disk file
 *
 * \param num_of_sectors this should be 400 or 800 for 40 or 80 track disks
 * \param name the disk name
 * \param diskfile the disk image file reference
 *
 * \return 0 on success or an error
 */
int dfs_format_diskfile(int num_of_sectors, const char * name, FILE * diskfile);

/**
 * \brief Extracts a file from a DFS disk image
 *
 * \param diskfile the disk image file reference
 * \param start_sector the start sector
 * \param file_length the file length
 * \param file the local file reference
 *
 * \return 0 on success or an error
 */
int dfs_extract_file(FILE * diskfile, const ACORN_FILE *acorn_filep, FILE * file);

/**
 * \brief Adds a file to the DFS disk image
 *
 * \param diskfile the disk image file reference
 * \param acorn_filep pointer to the file meta data
 * \param file the local file reference
 *
 * \return 0 on success or an error
 */
int dfs_add_file(FILE * diskfile, ACORN_FILE * acorn_filep, FILE * file);

#ifdef __cplusplus
}
#endif

#endif /* __DFS_H */
