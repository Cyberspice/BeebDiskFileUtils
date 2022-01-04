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

#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include "dfs.h"
#include "acornfs.h"
#include "debug.h"

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#define DFSUTILS_ERROR_FAILED          EXIT_FAILURE
#define DFSUTILS_DISKFILE_NOT_FOUND    2
#define DFSUTILS_NOT_A_DFSDISK         3
#define DFSUTILS_OPEN_FAILED           4
#define DFSUTILS_FILE_NOT_FOUND        5
#define DFSUTILS_NAME_TOO_LONG         6
#define DFSUTILS_INVALID_VALUE         7

static int tracks = 80;
static char * target_dir = NULL;

static void short_help(void) {
  fprintf(stderr,
    "dfsutils - Acorn DFS disk image utilities\n\n"
    "Usage: dfsutils diskfile\n"
    "   or: dfsutils --add [option] diskfile file load_address exec_address [locked]\n"
    "   or: dfsutils --extract [option] diskfile [file [file]...]\n"
    "   or: dfsutils --format [option] diskfile diskname\n"
    "   or: dfsutils --remove [option] diskfile file [file [file]...]\n"
    "   or: dfsutils --update [option] diskfile file load_address exec_address [locked]\n"
  );
}

static void help(void) {
  short_help();
  fprintf(stderr,
    "\nOptions:\n"
    "       --40           Simulate 40 track disk\n"
    "       --80           Simulate 80 track disk (default)\n"
    "   -a, --add          Add a file to the disk image\n"
    "   -d, --dir          Target directory\n"
    "   -f, --format       Creates a disk image (overwrites any existing file)\n"
    "   -h, --help         Display help\n"
    "   -r, --remove       Remove a file from the disk image\n"
    "   -u, --update       Update the properties of a file\n"
    "   -v, --verbose      Raise the verbosity (can be used more than once)\n"
    "   -x, --extract      Extract file(s)\n"
  );
}

static int dfs_error_to_exit_status(int dfserr) {
  switch (dfserr) {
    case DFS_ERROR_NONE:
      return EXIT_SUCCESS;
    case DFS_ERROR_NOT_A_DFS_DISK:
      return DFSUTILS_NOT_A_DFSDISK;
    case DFS_ERROR_FAILED:
      /* Drop through */
    default:
      return DFSUTILS_ERROR_FAILED;
  }
}

static int list_diskfile(int argc, char * argv[]) {
  static char * optionstr[] = {
    "None",
    "Load",
    "Run",
    "Exec"
  };

  ACORN_DIRECTORY * acorn_dirp;
  int ret;

  FILE * diskfile = fopen(argv[0], "rb");
  if (diskfile == NULL) {
    if (errno == ENOENT) {
      fprintf(stderr, "File not found: %s\n", argv[0]);
      return DFSUTILS_DISKFILE_NOT_FOUND;
    }

    fprintf(stderr, "Could not open: %s (%s)\n", argv[0], strerror(errno));
    return DFSUTILS_OPEN_FAILED;
  }

  ret = dfs_read_catalogue(diskfile, &acorn_dirp);
  if (ret != DFS_ERROR_NONE) {
    fclose(diskfile);
    return dfs_error_to_exit_status(ret);
  }

  fclose(diskfile);

  printf("Name   : %s\n", acorn_dirp->name);
  printf("Options: %d (%s)\n", acorn_dirp->options, optionstr[acorn_dirp->options]);
  printf("----------------------------------------------------------------\n");

  if (acorn_dirp->num_of_files) {
    ACORN_FILE * acorn_filep = &(acorn_dirp->files[0]);

    for (int i = 0; i < acorn_dirp->num_of_files; i++) {
      printf("  %-16s 0x%08x 0x%08x %10u %10u\n",
        acorn_filep->name,
        acorn_filep->load_address,
        acorn_filep->exec_address,
        acorn_filep->length,
        acorn_filep->start_sector);
      acorn_filep++;
    }

    printf("----------------------------------------------------------------\n");
  }

  printf("%d files\n", acorn_dirp->num_of_files);
  acornfs_free_directory(acorn_dirp);

  return EXIT_SUCCESS;
}

static int extract_file(FILE* diskfile, const char * dirname, const ACORN_FILE * acorn_filep) {
  static char path[PATH_MAX + 1];
  FILE * file;
  int ret;

  snprintf(path, sizeof(path), "%s/%s", dirname, acorn_filep->name);
  printf("Extracting: %s\n", path);

  file = fopen(path, "wb");
  if (file == NULL) {
    fprintf(stderr, "File error: %s\n", strerror(errno));
    return DFSUTILS_OPEN_FAILED;
  }

  ret = dfs_extract_file(diskfile, acorn_filep, file);
  if (ret != DFS_ERROR_NONE) {
    fclose(file);
    return dfs_error_to_exit_status(ret);
  }

  fclose(file);
  return EXIT_SUCCESS;
}

static int extract_diskfile(int argc, char * argv[]) {
  ACORN_DIRECTORY * acorn_dirp;
  ACORN_FILE * acorn_filep;
  char * dirname;
  int ret;
  int file_count = 0;

  FILE * diskfile = fopen(argv[0], "rb");
  if (diskfile == NULL) {
    if (errno == ENOENT) {
      fprintf(stderr, "File not found: %s\n", argv[0]);
      return DFSUTILS_DISKFILE_NOT_FOUND;
    }

    fprintf(stderr, "Could not open: %s (%s)\n", argv[0], strerror(errno));
    return DFSUTILS_OPEN_FAILED;
  }

  ret = dfs_read_catalogue(diskfile, &acorn_dirp);
  if (ret != DFS_ERROR_NONE) {
    fclose(diskfile);
    return dfs_error_to_exit_status(ret);
  }

  if (acorn_dirp->num_of_files == 0) {
    printf("Disk image empty! Nothing to extract.\n");
    fclose(diskfile);
    return 0;
  }

  if (target_dir != NULL) {
    dirname = target_dir;
  } else {
    dirname = acorn_dirp->name;
  }

  ret = mkdir(dirname, 0777);
  if (ret == -1) {
    fprintf(stderr, "Could not create: %s (%s)\n", dirname, strerror(errno));
    fclose(diskfile);
    return DFSUTILS_OPEN_FAILED;
  }

  printf("Output dir: %s\n", dirname);

  argc --;
  argv ++;

  ret = 0;
  if (argc) {
    while (argc) {
      int found = false;
      for (int i = 0; i < acorn_dirp->num_of_files; i++) {
        if (!strcmp(acorn_dirp->files[i].name, argv[0])) {
          found = true;
          acorn_filep = &(acorn_dirp->files[i]);
          break;
        }
      }

      if (!found) {
        fprintf(stderr, "File not found: %s\n", argv[0]);
        ret = DFSUTILS_FILE_NOT_FOUND;
        break;
      }

      ret = extract_file(diskfile, dirname, acorn_filep);
      if (ret != EXIT_SUCCESS) {
        break;
      }

      argc--;
      argv++;
      file_count++;
    }
  } else {
    acorn_filep = acorn_dirp->files;

    for (int i = 0; i < acorn_dirp->num_of_files; i++) {
      ret = extract_file(diskfile, dirname, acorn_filep);
      if (ret != EXIT_SUCCESS) {
        break;
      }

      acorn_filep++;
      file_count++;
    }
  }

  printf("%d files extracted\n", file_count);
  fclose(diskfile);

  return ret;
}

static int format_diskfile(int argc, char * argv[]) {
  FILE * diskfile = NULL;
  int ret;

  if (argc < 2) {
    short_help();
    return DFSUTILS_ERROR_FAILED;
  }

  if (strlen(argv[1]) > 12) {
    fprintf(stderr, "Diskname too long. Max 12 characters!");
    return DFSUTILS_NAME_TOO_LONG;
  }

  printf("Writing: %s\n", argv[1]);
  diskfile = fopen(argv[0], "wb");
  if (diskfile == NULL) {
    if (errno == ENOENT) {
      fprintf(stderr, "File not found: %s\n", argv[0]);
      return DFSUTILS_DISKFILE_NOT_FOUND;
    }

    fprintf(stderr, "Could not open: %s (%s)\n", argv[0], strerror(errno));
    return DFSUTILS_OPEN_FAILED;
  }

  ret = dfs_format_diskfile(tracks * DFS_SECTORS_PER_TRACK, argv[1], diskfile);

  fclose(diskfile);

  if (ret != DFS_ERROR_NONE) {
    return dfs_error_to_exit_status(ret);
  }

  return EXIT_SUCCESS;
}

static int add_file(int argc, char * argv[]) {
  ACORN_FILE acorn_file;
  FILE * diskfile = NULL;
  FILE * file = NULL;
  char * endptr;
  size_t count;
  int ret;

  if (argc < 4) {
    short_help();
    return DFSUTILS_ERROR_FAILED;
  }

  acorn_file.load_address = strtol(argv[2], &endptr, 0);
  if (*endptr) {
    fprintf(stderr, "Invalid load address: %s\n", argv[2]);
    return DFSUTILS_INVALID_VALUE;
  }

  acorn_file.exec_address = strtol(argv[3], &endptr, 0);
  if (*endptr) {
    fprintf(stderr, "Invalid exec address: %s\n", argv[2]);
    return DFSUTILS_INVALID_VALUE;
  }

  if (argc > 4) {
    if (strcmp(argv[4], "locked") == 0) {
      acorn_file.attributes = LOCKED;
    }
  }

  diskfile = fopen(argv[0], "rb+");
  if (diskfile == NULL) {
    if (errno == ENOENT) {
      fprintf(stderr, "File not found: %s\n", argv[0]);
      return DFSUTILS_DISKFILE_NOT_FOUND;
    }

    fprintf(stderr, "Could not open: %s (%s)\n", argv[0], strerror(errno));
    return DFSUTILS_OPEN_FAILED;
  }

  file = fopen(argv[1], "rb");
  if (file == NULL) {
    if (errno == ENOENT) {
      fprintf(stderr, "File not found: %s\n", argv[1]);
      return DFSUTILS_DISKFILE_NOT_FOUND;
    }

    fprintf(stderr, "Could not open: %s (%s)\n", argv[1], strerror(errno));
    return DFSUTILS_OPEN_FAILED;
  }


  ret = fseek(file, 0, SEEK_END);
  if (ret == -1) {
    fprintf(stderr, "Could not calculate file size: (%s)\n", strerror(errno));
    return DFSUTILS_ERROR_FAILED;
  }

  acorn_file.name = strdup(argv[1]);
  acorn_file.length = ftell(file);

  ret = dfs_add_file(diskfile, &acorn_file, file);

  free(acorn_file.name);
  fclose(diskfile);
  fclose(file);

  if (ret != DFS_ERROR_NONE) {
    return dfs_error_to_exit_status(ret);
  }

  return EXIT_SUCCESS;
}

int main(int argc, char * argv[]) {
  FILE * diskfile = NULL;
  int ch;
  bool do_add = false;
  bool do_format = false;
  bool do_extract = false;
  bool do_remove = false;
  bool do_update = false;
  int actions = 0;

  static struct option longopts[] = {
    { "40",        no_argument,       &tracks,    40},
    { "80",        no_argument,       &tracks,    80},
    { "add",       no_argument,       NULL,       'a'},
    { "dir",       required_argument, NULL,       'd'},
    { "extract",   no_argument,       NULL,       'x'},
    { "format",    no_argument,       NULL,       'f'},
    { "help",      no_argument,       NULL,       'h'},
    { "remove",    no_argument,       NULL,       'r'},
    { "update",    no_argument,       NULL,       'u'},
    { "verbose",   no_argument,       NULL,       'v'},
    { NULL,        0,                 NULL,       0  }
  };

  while ((ch = getopt_long(argc, argv, "ad:fhruvx", longopts, NULL)) != -1) {
    switch(ch) {
      case 0: /* Track values */
        break;
      case 'a': /* Add */
        do_add = true;
        actions++;
        break;
      case 'd': /* Target directory */
        target_dir = strdup(optarg);
        break;
      case 'f': /* Format */
        do_format = true;
        actions++;
        break;
      case 'h': /* Help */
        help();
        exit(EXIT_SUCCESS);
        break;
      case 'r': /* Help */
        do_remove = true;
        actions++;
        break;
      case 'u': /* Update */
        do_update = true;
        actions++;
        break;
      case 'v': /* verbosity */
        verbosity++;
        break;
      case 'x': /* extract */
        do_extract = true;
        actions++;
        break;
      default:
        help();
        exit(DFSUTILS_ERROR_FAILED);
    }
  }

  argc -= optind;
  argv += optind;

  /* Check we're not trying to do two things at once */
  if (actions > 1) {
    help();
    exit(DFSUTILS_ERROR_FAILED);
  }

  if (argc < 1) {
    short_help();
    exit(DFSUTILS_ERROR_FAILED);
  }

  if (do_add) {
    return add_file(argc, argv);
  }

  if (do_extract) {
    return extract_diskfile(argc, argv);
  }

  if (do_format) {
    return format_diskfile(argc, argv);
  }

  if (do_remove) {
    return 0;
  }

  if (do_update) {
    return 0;
  }

  return list_diskfile(argc, argv);
}
