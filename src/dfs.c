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
#include <string.h>
#include <errno.h>
#include "acornfs.h"
#include "dfs.h"
#include "debug.h"

#ifdef __riscos
#define min(a,b) \
         ((a) < (b) ? (a) : (b))
#else
#define min(a,b) \
       ({ __typeof__ (a) _a = (a); \
           __typeof__ (b) _b = (b); \
         _a < _b ? _a : _b; })
#endif

static int get_number_of_sectors(const DFS_SECTOR_1 * sector1p) {
  uint16_t sector_high;
  uint16_t sector_low;
  int num_of_sectors;

  sector_high = (uint16_t)(sector1p->disk_name_1.num_of_sectors_high & DFS_NUM_OF_SECTORS_LOW_MASK);
  sector_low  = (uint16_t)(sector1p->disk_name_1.num_of_sectors_low);
  return (sector_high * 0x100) + sector_low;
}

static int get_boot_options(const DFS_SECTOR_1 * sector1p) {
  return ((sector1p->disk_name_1.num_of_sectors_high & DFS_BOOT_OPTIONS_MASK) / 0x10);
}

static int get_number_of_files(const DFS_SECTOR_1 * sector1p) {
  return ((sector1p->disk_name_1.num_of_files & DFS_NUM_OF_FILES_MASK) / 0x08);
}

static void set_number_of_files(DFS_SECTOR_1 * sector1p, int num_of_files) {
  sector1p->disk_name_1.num_of_files =
    (sector1p->disk_name_1.num_of_files & (~DFS_NUM_OF_FILES_MASK)) |
    (num_of_files << DFS_NUM_OF_FILES_SHIFT);
}

void increment_cycle_number(DFS_SECTOR_1 * sector1p) {
  sector1p->disk_name_1.cycle_number ++;
  if ((sector1p->disk_name_1.cycle_number & 0x0f) == 0x0a) {
    sector1p->disk_name_1.cycle_number += 6;
    if (sector1p->disk_name_1.cycle_number == 0xa0) {
      sector1p->disk_name_1.cycle_number = 0;
    }
  }
}

static char * get_disk_name(const DFS_SECTOR_0 * sector0p, const DFS_SECTOR_1 * sector1p) {
  char diskname[DFS_MAX_DISK_NAME_LEN + 1];
  int i;

  memset(diskname, 0, sizeof(diskname));
  memcpy(
    diskname,
    sector0p->disk_name_0.diskname_0,
    sizeof(sector0p->disk_name_0.diskname_0));
  memcpy(
    diskname + sizeof(sector0p->disk_name_0.diskname_0),
    sector1p->disk_name_1.diskname_1,
    sizeof(sector1p->disk_name_1.diskname_1));

  for (i = 0; i <= DFS_MAX_DISK_NAME_LEN; i++) {
    if (diskname[i] == '\0' || diskname[i] == ' ') {
      diskname[i] = '\0';
      break;
    }
  }

  return strdup(diskname);
}

static int dfs_get_file_name_and_dir(const char * file_name, char ** namepp, char * dirp) {
  char * name = strdup(file_name);
  char * separator = strchr(name, '.'); /* First '.' */
  char * p;

  if (namepp == NULL || dirp == NULL) {
    return DFS_ERROR_FAILED;
  }

  /* Directory (extension) should be one character */
  if (separator && strlen(separator + 1) > DFS_MAX_DIR_NAME_LEN) {
    free(name);

    if (DEBUG_LEVEL_ERROR) fprintf(stderr, "Directory name too long!\n");
    return DFS_ERROR_INVALID_FILE_NAME;
  }

  /* Separate name from directory (extension) and check validity of directory name */
  if (separator) {
    separator[0] = '\0';

    if ((separator[1] == ':') || (separator[1] == '\"') || (separator[1] == '#') ||
        (separator[1] == '*') || (separator[1] == '.') || (separator[1] == ' ')) {
      free(name);

      if (DEBUG_LEVEL_ERROR) fprintf(stderr, "Invalid char in dir name!\n");
      return DFS_ERROR_INVALID_FILE_NAME;
    }
  }

  /* Name must be 7 characters or less */
  if (strlen(name) > DFS_MAX_FILE_NAME_LEN) {
    free(name);

    if (DEBUG_LEVEL_ERROR) fprintf(stderr, "File name too long!\n");
    return DFS_ERROR_INVALID_FILE_NAME;
  }

  /* Test for invalid chars.  '.' will have already been tested above */
  for (p = name; *p; p++) {
    if ((*p == ':') || (*p == '\"') || (*p == '#') || (*p == '*') || (*p == ' ')) {
      free(name);

      if (DEBUG_LEVEL_ERROR) fprintf(stderr, "Invalid char in file name: \'%c\'\n", *p);
      return DFS_ERROR_INVALID_FILE_NAME;
    }
  }

  *namepp = name;

  /* No separator defaults to $ */
  if (separator) {
    if (separator[1]) {
      *dirp = separator[1];
    } else {
      *dirp = '$';
    }
  } else {
    *dirp = '$';
  }

  return 0;
}

static char * get_file_name(const DFS_FILE_NAME * filenamep) {
  /* Filename size plus '.' plus directory character */
  char filename[DFS_MAX_FILE_NAME_LEN + 3];
  int i;

  memset(filename, 0, sizeof(filename));
  memcpy(filename, filenamep->filename, DFS_MAX_FILE_NAME_LEN);

  for (i = 0; i <= DFS_MAX_FILE_NAME_LEN; i++) {
    if (filename[i] == '\0' || filename[i] == ' ') {
      if (filenamep->directory != '$') {
        filename[i++] = '.';
        filename[i++] = (filenamep->directory) % DFS_DIR_NAME_MASK;
      }
      filename[i] = '\0';
      break;
    }
  }

  return strdup(filename);
}

static void get_file_info(const DFS_FILE_NAME * filenamep, const DFS_FILE_PARAMS * fileparamsp, ACORN_FILE * acorn_filep) {
  acorn_filep->name = get_file_name(filenamep);

  if ((filenamep->directory & DFS_LOCK_BIT) == DFS_LOCK_BIT) {
    acorn_filep->attributes = LOCKED;
  }

  acorn_filep->load_address =
    (uint32_t)fileparamsp->load_address_low +
    ((uint32_t)fileparamsp->load_address_high * 0x100) +
    ((uint32_t)((fileparamsp->start_sector_high & DFS_LOAD_ADDRESS_BIT_17_18_MASK) >> DFS_LOAD_ADDRESS_SHIFT) * 0x10000);

  /* If load address has bits 17 and 18 set then OR with 0xffff0000 (IO memory) */
  if ((fileparamsp->start_sector_high & DFS_LOAD_ADDRESS_BIT_17_18_MASK) == DFS_LOAD_ADDRESS_BIT_17_18_MASK) {
    acorn_filep->load_address |= 0xffff0000;
  }

  acorn_filep->exec_address =
    (uint32_t)fileparamsp->exec_address_low +
    ((uint32_t)fileparamsp->exec_address_high * 0x100) +
    ((uint32_t)((fileparamsp->start_sector_high & DFS_EXEC_ADDRESS_BIT_17_18_MASK) >> DFS_LOAD_ADDRESS_SHIFT) * 0x10000);

  /* If exec address has bits 17 and 18 set then OR with 0xffff0000 (IO memory) */
  if ((fileparamsp->start_sector_high & DFS_EXEC_ADDRESS_BIT_17_18_MASK) == DFS_EXEC_ADDRESS_BIT_17_18_MASK) {
    acorn_filep->exec_address |= 0xffff0000;
  }

  acorn_filep->length =
    (uint32_t)fileparamsp->length_low +
    ((uint32_t)fileparamsp->length_high * 0x100) +
    ((uint32_t)((fileparamsp->start_sector_high & DFS_FILE_LENGTH_BIT_17_18_MASK) >> DFS_FILE_LENGTH_SHIFT) * 0x10000);

  acorn_filep->start_sector =
    (uint32_t)fileparamsp->start_sector_low +
    (((uint32_t)fileparamsp->start_sector_high & DFS_START_SECTOR_HIGH_MASK) * 0x100);
}

static int set_file_params(const ACORN_FILE * acorn_filep, DFS_FILE_NAME * filenamep, DFS_FILE_PARAMS * fileparamsp) {
  if (acorn_filep->attributes == LOCKED) {
    filenamep->directory |= DFS_LOCK_BIT;
  }

  fileparamsp->start_sector_high  = 0;

  fileparamsp->load_address_low   = (acorn_filep->load_address & 0xff);
  fileparamsp->load_address_high  = (acorn_filep->load_address / 0x100) & 0xff;
  fileparamsp->start_sector_high |= ((acorn_filep->load_address / 0x10000) & 0x03) << DFS_LOAD_ADDRESS_SHIFT;

  fileparamsp->exec_address_low   = (acorn_filep->exec_address & 0xff);
  fileparamsp->exec_address_high  = (acorn_filep->exec_address / 0x100) & 0xff;
  fileparamsp->start_sector_high |= ((acorn_filep->exec_address / 0x10000) & 0x03) << DFS_EXEC_ADDRESS_SHIFT;

  fileparamsp->length_low         = (acorn_filep->length & 0xff);
  fileparamsp->length_high        = (acorn_filep->length / 0x100) & 0xff;
  fileparamsp->start_sector_high |= ((acorn_filep->length / 0x10000) & 0x03) << DFS_FILE_LENGTH_SHIFT;

  fileparamsp->start_sector_low   = (acorn_filep->start_sector & 0xff);
  fileparamsp->start_sector_high |= ((acorn_filep->start_sector / 0x100) & 0x03);

  return DFS_ERROR_NONE;
}

static int read_catalogue_sectors(FILE * diskfile, uint8_t * sector0p, uint8_t * sector1p, int * num_of_sectorsp) {
  size_t count = fread(sector0p, DFS_SECTOR_SIZE, 1, diskfile);
  if (count == 0) {
    if (DEBUG_LEVEL(DEBUG_LEVEL_ERROR)) fprintf(stdout, "Could not read sector 0\n");
    return DFS_ERROR_NOT_A_DFS_DISK;
  }

  count = fread(sector1p, DFS_SECTOR_SIZE, 1, diskfile);
  if (count == 0) {
    if (DEBUG_LEVEL(DEBUG_LEVEL_ERROR)) fprintf(stdout, "Could not read sector 0\n");
    return DFS_ERROR_NOT_A_DFS_DISK;
  }

  *num_of_sectorsp = get_number_of_sectors((DFS_SECTOR_1 *)sector1p);

  if (*num_of_sectorsp != DFS_40_TRACK_NUM_OF_SECTORS && *num_of_sectorsp != DFS_80_TRACK_NUM_OF_SECTORS) {
    if (DEBUG_LEVEL(DEBUG_LEVEL_WARNING)) fprintf(stdout, "Invalid number of sectors in disk: %u\n", *num_of_sectorsp);
    return DFS_ERROR_NOT_A_DFS_DISK;
  }

  return 0;
}

static int get_first_free_sector(DFS_SECTOR_1 * sector1p, int * free_sectorp) {
  DFS_FILE_PARAMS * file_params = &(sector1p->file_params[0]);
  int free_sector = 2; /* Empty disk is sector 2 */
  int file_length = 0;
  int i;

  /* Find last file on the disk */
  for (i = 0; i < sector1p->disk_name_1.num_of_files; i++) {
    int start_sector =
      ((uint32_t)sector1p->file_params[i].start_sector_low) +
      (((uint32_t)sector1p->file_params[i].start_sector_high & DFS_START_SECTOR_HIGH_MASK) * 0x100);

    if (start_sector > free_sector) {
      free_sector = start_sector;
      file_params = &(sector1p->file_params[i]);
    }
  }

  /* Find first free sector after this file */
  file_length =
    (uint32_t)file_params->length_low +
    ((uint32_t)file_params->length_high * 0x100) +
    ((uint32_t)((file_params->start_sector_high & DFS_FILE_LENGTH_BIT_17_18_MASK) / 16) * 0x10000);

  /* Free sector */
  *free_sectorp = free_sector + (file_length / DFS_SECTOR_SIZE) + 1;

  return 0;
}

/**
 * \brief reads the catalogue from a DFS disk
 *
 * This function reads the catalogue from a DFS disk. It assumes the file
 * pointer is set to the start of the disk image file.  The function returns
 * a pointer to an ACORN_DIRECTORY. This needs to be freed with
 * acornfs_free_directory() when no longer required.
 *
 * \param diskfile the disk image file reference
 * \param acorn_dirpp pointer in which to return the acorn directory
 * \return 0 on success or an error
 */
int dfs_read_catalogue(FILE * diskfile, ACORN_DIRECTORY ** acorn_dirpp) {
  uint8_t sector0[DFS_SECTOR_SIZE];
  uint8_t sector1[DFS_SECTOR_SIZE];
  DFS_SECTOR_0 * sector0p;
  DFS_SECTOR_1 * sector1p;
  int num_of_sectors;
  int boot_options;
  int num_of_files;
  ACORN_DIRECTORY * acorn_dirp;
  DFS_FILE_NAME * filenamep;
  DFS_FILE_PARAMS * fileparamsp;
  ACORN_FILE * acorn_filep;
  int ret;
  int i;

  if (acorn_dirpp == NULL) {
    return DFS_ERROR_FAILED;
  }

  ret = read_catalogue_sectors(diskfile, sector0, sector1, &num_of_sectors);
  if (ret != DFS_ERROR_NONE) {
    return ret;
  }

  sector0p = (DFS_SECTOR_0*)sector0;
  sector1p = (DFS_SECTOR_1*)sector1;

  num_of_files = get_number_of_files(sector1p);
  if (DEBUG_LEVEL(DEBUG_LEVEL_INFO)) fprintf(stderr, "Number of files: %u\n", num_of_files);

  acorn_dirp =
    (ACORN_DIRECTORY*)malloc(sizeof(ACORN_DIRECTORY) + (sizeof(ACORN_FILE) * (size_t)num_of_sectors));
  if (acorn_dirp == NULL) {
    perror("dfsutils");
    return DFS_ERROR_FAILED;
  }

  acorn_dirp->parent = NULL;
  acorn_dirp->num_of_files = num_of_files;

  acorn_dirp->name = get_disk_name(sector0p, sector1p);
  if (DEBUG_LEVEL(DEBUG_LEVEL_INFO)) fprintf(stderr, "Disk name: %s\n", acorn_dirp->name);

  acorn_dirp->options = get_boot_options(sector1p);
  if (DEBUG_LEVEL(DEBUG_LEVEL_INFO)) fprintf(stderr, "Boot options: 0x%02x\n", acorn_dirp->options);

  filenamep = sector0p->file_names;
  fileparamsp = sector1p->file_params;
  acorn_filep = acorn_dirp->files;

  for(i = 0; i < num_of_files; i++) {
    get_file_info(filenamep++, fileparamsp++, acorn_filep++);
  }

  *acorn_dirpp = acorn_dirp;
  return DFS_ERROR_NONE;
}

/**
 * \brief Creates an empty DFS disk file
 *
 * \param num_of_sectors this should be 400 or 800 for 40 or 80 track disks
 * \param name the disk name
 * \param diskfile the disk image file reference
 *
 * \return 0 on success or an error
 */
int dfs_format_diskfile(int num_of_sectors, const char * name, FILE * diskfile) {
  DFS_SECTOR_0 sector0;
  DFS_SECTOR_1 sector1;
  uint8_t sector2[DFS_SECTOR_SIZE];
  size_t namelen = strlen(name);
  size_t count = 0;
  int i;

  if (num_of_sectors != DFS_40_TRACK_NUM_OF_SECTORS && num_of_sectors != DFS_80_TRACK_NUM_OF_SECTORS) {
    if (DEBUG_LEVEL(DEBUG_LEVEL_ERROR)) fprintf(stderr, "Invalid number of sectors: %u\n", num_of_sectors);
    return DFS_ERROR_INVALID_NUMBER_OF_SECTORS;
  }

  /* Clear the catalogue sectors */
  memset(&sector0, 0, sizeof(sector0));
  memset(&sector1, 0, sizeof(sector1));

  /* Set the disk name */
  memcpy(
    sector0.disk_name_0.diskname_0,
    name,
    min(sizeof(sector0.disk_name_0.diskname_0), namelen));
  if (namelen > sizeof(sector0.disk_name_0.diskname_0)) {
    memcpy(
      sector1.disk_name_1.diskname_1,
      name + sizeof(sector0.disk_name_0.diskname_0),
      min(sizeof(sector1.disk_name_1.diskname_1), namelen - sizeof(sector0.disk_name_0.diskname_0)));
  }

  /* Set the disk params:
       Cycle number 1
       Number of files 0
       Disk option 0x00
       Number of sectors 400 or 800 */
  sector1.disk_name_1.cycle_number = 1;
  sector1.disk_name_1.num_of_files = 0;
  sector1.disk_name_1.num_of_sectors_high = num_of_sectors / 0x100;
  sector1.disk_name_1.num_of_sectors_low  = (uint8_t)(num_of_sectors & 0xff);

  memset(sector2, 0, sizeof(sector2));

  /* Write the DFS catalogue */
  count = fwrite(&sector0, sizeof(sector0), 1, diskfile);
  if (count == 0) {
    if (DEBUG_LEVEL(DEBUG_LEVEL_ERROR)) fprintf(stdout, "Could not write sector 0\n");
    return DFS_ERROR_FAILED;
  }

  count = fwrite(&sector1, sizeof(sector1), 1, diskfile);
  if (count == 0) {
    if (DEBUG_LEVEL(DEBUG_LEVEL_ERROR)) fprintf(stderr, "Could not write sector 1\n");
    return DFS_ERROR_FAILED;
  }

  /* Pad out to the number of sectors */
  for (i = 2; i < num_of_sectors; i++) {
    count = fwrite(sector2, sizeof(sector2), 1, diskfile);
    if (count == 0) {
      if (DEBUG_LEVEL(DEBUG_LEVEL_ERROR)) fprintf(stderr, "Could not write data sector: %u\n", i);
      return DFS_ERROR_FAILED;
    }
  }

  return DFS_ERROR_NONE;
}

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
int dfs_extract_file(FILE * diskfile, const ACORN_FILE *acorn_filep, FILE * file) {
  uint8_t sector[DFS_SECTOR_SIZE];
  int len = (int)acorn_filep->length;
  size_t count = 0;

  int ret = fseek(diskfile, (long)((acorn_filep->start_sector) * DFS_SECTOR_SIZE), SEEK_SET);
  if (ret == -1) {
    if (DEBUG_LEVEL(DEBUG_LEVEL_ERROR)) fprintf(stderr, "Could not open target file!\n");
    return DFS_ERROR_FAILED;
  }

  while (len > 0) {
    count = fread(sector, min(sizeof(sector), (size_t)len), 1, diskfile);
    if (count == 0) {
      if (DEBUG_LEVEL(DEBUG_LEVEL_ERROR)) fprintf(stderr, "Could not read sector!\n");
      return DFS_ERROR_FAILED;
    }

    fwrite(sector, min(sizeof(sector), len), 1, file);
    len -= sizeof(sector);
  }

  return DFS_ERROR_NONE;
}

static int dfs_add_file_data(FILE * diskfile, FILE * file, int start_sector) {
  uint8_t sector[DFS_SECTOR_SIZE];
  size_t count;

  int ret = fseek(file, 0, SEEK_SET);
  if (ret == -1) {
    if (DEBUG_LEVEL(DEBUG_LEVEL_ERROR)) fprintf(stderr, "Could not update disk image: %s\n", strerror(errno));
    return DFS_ERROR_FAILED;
  }

  ret = fseek(diskfile, start_sector * DFS_SECTOR_SIZE, SEEK_SET);
  if (ret == -1) {
    if (DEBUG_LEVEL(DEBUG_LEVEL_ERROR)) fprintf(stderr, "Could not update disk image: %s\n", strerror(errno));
    return DFS_ERROR_FAILED;
  }

  while (!feof(file)) {
    count = fread(sector, sizeof(uint8_t), DFS_SECTOR_SIZE, file);
    if (count < 0) {
      if (DEBUG_LEVEL(DEBUG_LEVEL_ERROR)) fprintf(stderr, "Could not update disk image: %s\n", strerror(errno));
      return DFS_ERROR_FAILED;
    }

    if (count > 0) {
      count = fwrite(sector, sizeof(uint8_t), count, diskfile);
      if (count < 0) {
        if (DEBUG_LEVEL(DEBUG_LEVEL_ERROR)) fprintf(stderr, "Could not update disk image: %s\n", strerror(errno));
        return DFS_ERROR_FAILED;
      }
    }
  }

  return 0;
}

/**
 * \brief Adds a file to the DFS disk image
 *
 * \param diskfile the disk image file reference
 * \param acorn_filep pointer to the file meta data
 * \param file the local file reference
 *
 * \return 0 on success or an error
 */
int dfs_add_file(FILE * diskfile, ACORN_FILE * acorn_filep, FILE * file) {
  char dfs_name[DFS_MAX_FILE_NAME_LEN];
  uint8_t sector0[DFS_SECTOR_SIZE];
  uint8_t sector1[DFS_SECTOR_SIZE];
  DFS_SECTOR_0 * sector0p;
  DFS_SECTOR_1 * sector1p;
  size_t count;
  char * name;
  char dir;
  int name_len;
  int num_of_sectors;
  int first_free_sector;
  int free_space;
  int num_of_files;
  int ret;
  int i;

  ret = read_catalogue_sectors(diskfile, sector0, sector1, &num_of_sectors);
  if (ret != DFS_ERROR_NONE) {
    return ret;
  }

  sector0p = (DFS_SECTOR_0*)sector0;
  sector1p = (DFS_SECTOR_1*)sector1;

  num_of_files = get_number_of_files(sector1p);
  if (num_of_files >= DFS_MAX_FILES) {
    if (DEBUG_LEVEL(DEBUG_LEVEL_INFO)) fprintf(stderr, "Number of files: %u\n", num_of_files);
    return DFS_ERROR_DISK_FULL;
  }

  ret = dfs_get_file_name_and_dir(acorn_filep->name, &name, &dir);
  if (ret != DFS_ERROR_NONE) {
    return ret;
  }

  memset(dfs_name, ' ', sizeof(dfs_name));
  memcpy(dfs_name, name, strlen(name));
  free(name);

  for (i = 0; i < num_of_files; i++) {
    if (dir != (sector0p->file_names[i].directory & DFS_DIR_NAME_MASK)) {
      continue;
    }

    if (memcmp(dfs_name, sector0p->file_names[i].filename, sizeof(dfs_name)) == 0) {
      if (DEBUG_LEVEL(DEBUG_LEVEL_ERROR)) fprintf(stderr, "File exists!\n");
      return DFS_ERROR_FILE_EXISTS;
    }
  }

  get_first_free_sector(sector1p, &first_free_sector);

  free_space = (num_of_sectors - first_free_sector) * DFS_SECTOR_SIZE;
  if (free_space < acorn_filep->length) {
    if (DEBUG_LEVEL(DEBUG_LEVEL_ERROR)) fprintf(stderr, "Disk image full!\n");
    return DFS_ERROR_DISK_FULL;
  }

  memcpy(&(sector0p->file_names[num_of_files].filename), dfs_name, sizeof(dfs_name));
  sector0p->file_names[num_of_files].directory = dir;

  acorn_filep->start_sector = first_free_sector;

  ret = set_file_params(acorn_filep, &(sector0p->file_names[num_of_files]), &(sector1p->file_params[num_of_files]));
  if (ret != DFS_ERROR_NONE) {
    return ret;
  }

  /* Write file first before updating catalogue in case of error */
  ret = dfs_add_file_data(diskfile, file, first_free_sector);
  if (ret != DFS_ERROR_NONE) {
    return ret;
  }

  ret = fseek(diskfile, 0, SEEK_SET);
  if (ret == -1) {
    if (DEBUG_LEVEL(DEBUG_LEVEL_ERROR)) fprintf(stderr, "Could not update disk image: %s\n", strerror(errno));
    return DFS_ERROR_FAILED;
  }

  set_number_of_files(sector1p, num_of_files + 1);
  increment_cycle_number(sector1p);

  count = fwrite(&sector0, sizeof(sector0), 1, diskfile);
  if (count == 0) {
    if (DEBUG_LEVEL(DEBUG_LEVEL_ERROR)) fprintf(stderr, "Could not write sector 0\n");
    return DFS_ERROR_FAILED;
  }

  count = fwrite(&sector1, sizeof(sector1), 1, diskfile);
  if (count == 0) {
    if (DEBUG_LEVEL(DEBUG_LEVEL_ERROR)) fprintf(stderr, "Could not write sector 1\n");
    return DFS_ERROR_FAILED;
  }

  return DFS_ERROR_NONE;
}
