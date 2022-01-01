
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "acornfs.h"
#include "dfs.h"
#include "debug.h"

#define min(a,b) \
       ({ __typeof__ (a) _a = (a); \
           __typeof__ (b) _b = (b); \
         _a < _b ? _a : _b; })

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

static char * get_disk_name(const DFS_SECTOR_0 * sector0p, const DFS_SECTOR_1 * sector1p) {
  char diskname[DFS_MAX_DISK_NAME_LEN + 1];

  memset(diskname, 0, sizeof(diskname));
  memcpy(
    diskname,
    sector0p->disk_name_0.diskname_0,
    sizeof(sector0p->disk_name_0.diskname_0));
  memcpy(
    diskname + sizeof(sector0p->disk_name_0.diskname_0),
    sector1p->disk_name_1.diskname_1,
    sizeof(sector1p->disk_name_1.diskname_1));

  for (int i = 0; i <= DFS_MAX_DISK_NAME_LEN; i++) {
    if (diskname[i] == '\0' || diskname[i] == ' ') {
      diskname[i] = '\0';
      break;
    }
  }

  return strdup(diskname);
}

static char * get_file_name(const DFS_FILE_NAME * filenamep) {
  /* Filename size plus '.' plus directory character */
  char filename[DFS_MAX_FILE_NAME_LEN + 3];

  memset(filename, 0, sizeof(filename));
  memcpy(filename, filenamep->filename, DFS_MAX_FILE_NAME_LEN);

  for (int i = 0; i <= DFS_MAX_FILE_NAME_LEN; i++) {
    if (filename[i] == '\0' || filename[i] == ' ') {
      if (filenamep->directory != '$') {
        filename[i++] = '.';
        filename[i++] = filenamep->directory;
      }
      filename[i] = '\0';
      break;
    }
  }

  return strdup(filename);
}

static void get_file_info(const DFS_FILE_NAME * filenamep, const DFS_FILE_PARAMS * fileparamsp, ACORN_FILE * acorn_filep) {
  acorn_filep->name = get_file_name(filenamep);
  acorn_filep->load_address =
    (uint32_t)fileparamsp->load_address_low +
    ((uint32_t)fileparamsp->load_address_high * 0x100) +
    ((uint32_t)((fileparamsp->start_sector_high & DFS_LOAD_ADDRESS_BIT_17_18_MASK) / 4) * 0x10000);

  /* If load address has bits 17 and 18 set then OR with 0xffff0000 (IO memory) */
  if ((fileparamsp->start_sector_high & DFS_LOAD_ADDRESS_BIT_17_18_MASK) == DFS_LOAD_ADDRESS_BIT_17_18_MASK) {
    acorn_filep->load_address |= 0xffff0000;
  }

  acorn_filep->exec_address =
    (uint32_t)fileparamsp->exec_address_low +
    ((uint32_t)fileparamsp->exec_address_high * 0x100) +
    ((uint32_t)((fileparamsp->start_sector_high & DFS_EXEC_ADDRESS_BIT_17_18_MASK) / 64) * 0x10000);

  /* If exec address has bits 17 and 18 set then OR with 0xffff0000 (IO memory) */
  if ((fileparamsp->start_sector_high & DFS_EXEC_ADDRESS_BIT_17_18_MASK) == DFS_EXEC_ADDRESS_BIT_17_18_MASK) {
    acorn_filep->exec_address |= 0xffff0000;
  }

  acorn_filep->length =
    (uint32_t)fileparamsp->length_low +
    ((uint32_t)fileparamsp->length_high * 0x100) +
    ((uint32_t)((fileparamsp->start_sector_high & DFS_FILE_LENGTH_BIT_17_18_MASK) / 16) * 0x10000);

  acorn_filep->start_sector =
    (uint32_t)fileparamsp->start_sector_low +
    ((uint32_t)fileparamsp->start_sector_high & DFS_START_SECTOR_HIGH_MASK * 0x100);
}

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
int dfs_read_catalogue(FILE * diskfile, ACORN_DIRECTORY ** acorn_dirpp) {
  uint8_t sector0[256];
  uint8_t sector1[256];
  DFS_SECTOR_0 * sector0p;
  DFS_SECTOR_1 * sector1p;
  int num_of_sectors;
  int boot_options;
  int num_of_files;
  ACORN_DIRECTORY * acorn_dirp;
  DFS_FILE_NAME * filenamep;
  DFS_FILE_PARAMS * fileparamsp;
  ACORN_FILE * acorn_filep;
  size_t count;

  if (acorn_dirpp == NULL) {
    return DFS_ERROR_FAILED;
  }

  count = fread(sector0, sizeof(sector0), 1, diskfile);
  if (count == 0) {
    if (DEBUG_LEVEL(DEBUG_LEVEL_ERROR)) fprintf(stdout, "Could not read sector 0\n");
    return DFS_ERROR_NOT_A_DFS_DISK;
  }

  count = fread(sector1, sizeof(sector1), 1, diskfile);
  if (count == 0) {
    if (DEBUG_LEVEL(DEBUG_LEVEL_ERROR)) fprintf(stdout, "Could not read sector 0\n");
    return DFS_ERROR_NOT_A_DFS_DISK;
  }

  sector0p = (DFS_SECTOR_0*)sector0;
  sector1p = (DFS_SECTOR_1*)sector1;

  num_of_sectors = get_number_of_sectors(sector1p);

  if (num_of_sectors != DFS_40_TRACK_NUM_OF_SECTORS && num_of_sectors != DFS_80_TRACK_NUM_OF_SECTORS) {
    if (DEBUG_LEVEL(DEBUG_LEVEL_WARNING)) fprintf(stdout, "Invalid number of sectors in disk: %u\n", num_of_sectors);
    return DFS_ERROR_NOT_A_DFS_DISK;
  }

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

  for(int i = 0; i < num_of_files; i++) {
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
    if (DEBUG_LEVEL(DEBUG_LEVEL_ERROR)) fprintf(stdout, "Could not write sector 1\n");
    return DFS_ERROR_FAILED;
  }

  /* Pad out to the number of sectors */
  for (int i = 2; i < num_of_sectors; i++) {
    count = fwrite(sector2, sizeof(sector2), 1, diskfile);
    if (count == 0) {
      if (DEBUG_LEVEL(DEBUG_LEVEL_ERROR)) fprintf(stdout, "Could not write data sector: %u\n", i);
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
int dfs_extract_file(FILE * diskfile, ACORN_FILE *acorn_filep, FILE * file) {
  uint8_t sector[DFS_SECTOR_SIZE];
  int len = (int)acorn_filep->length;
  size_t count = 0;

  int ret = fseek(diskfile, (long)((acorn_filep->start_sector) * DFS_SECTOR_SIZE), SEEK_SET);
  if (ret == -1) {
    if (DEBUG_LEVEL(DEBUG_LEVEL_ERROR)) fprintf(stdout, "Could not open target file!\n");
    return DFS_ERROR_FAILED;
  }

  while (len > 0) {
    count = fread(sector, min(sizeof(sector), (size_t)len), 1, diskfile);
    if (count == 0) {
      if (DEBUG_LEVEL(DEBUG_LEVEL_ERROR)) fprintf(stdout, "Could not read sector!\n");
      return DFS_ERROR_FAILED;
    }

    fwrite(sector, min(sizeof(sector), len), 1, file);
    len -= sizeof(sector);
  }

  return DFS_ERROR_NONE;
}

