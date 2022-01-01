#include <sys/stat.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include <unistd.h>

#include "dfs.h"
#include "acornfs.h"
#include "debug.h"

static int tracks = 80;
static char * target_dir = NULL;

static void short_help(void) {
  fprintf(stderr,
    "dfsutils - Acorn DFS disk image utilities\n\n"
    "Usage: dfsutils diskfile\n"
    "   or: dfsutils --add [option] diskfile [file [file]...]\n"
    "   or: dfsutils --extract [option] diskfile [file [file]...]\n"
    "   or: dfsutils --format [option] diskfile diskname [file [file]...]\n"
    "   or: dfsutils --remove [option] diskfile file [file [file]...]\n"
    "   or: dfsutils --update [option] diskfile file load_address exec_address file_opts\n"
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
    "   -v, --verbose      Raise the verbosity\n"
    "   -x, --extract      Extract file(s)\n"
  );
}

static int list_diskfile(int argc, char * argv[]) {
  static char * optionstr[] = {
    "None",
    "Load",
    "Run",
    "Exec"
  };

  ACORN_DIRECTORY * acorn_dirp;

  FILE * diskfile = fopen(argv[0], "rb");
  if (diskfile == NULL) {
    perror("dfsutils");
    return 1;
  }

  if (dfs_read_catalogue(diskfile, &acorn_dirp)) {
    fclose(diskfile);
    return 1;
  }

  fclose(diskfile);

  printf("Name   : %s\n", acorn_dirp->name);
  printf("Options: %d (%s)\n", acorn_dirp->options, optionstr[acorn_dirp->options]);
  printf("----------------------------------------------------------------\n");

  if (acorn_dirp->num_of_files) {
    ACORN_FILE * acorn_filep = acorn_dirp->files;

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

  return 0;
}

static int extract_diskfile(int argc, char * argv[]) {
  ACORN_DIRECTORY * acorn_dirp;

  FILE * diskfile = fopen(argv[0], "rb");
  if (diskfile == NULL) {
    perror("dfsutils");
    return 1;
  }

  if (dfs_read_catalogue(diskfile, &acorn_dirp)) {
    fclose(diskfile);
    return 1;
  }

  if (acorn_dirp->num_of_files) {
    ACORN_FILE * acorn_filep = acorn_dirp->files;
    char path[256];
    FILE * file;

    mkdir(acorn_dirp->name, 0777);
    printf("Output dir: %s\n", acorn_dirp->name);

    for (int i = 0; i < acorn_dirp->num_of_files; i++) {
      snprintf(path, sizeof(path), "%s/%s", acorn_dirp->name, acorn_filep->name);
      printf("Extracting: %s\n", path);

      file = fopen(path, "wb");
      if (file == NULL) {
        fprintf(stderr, "File error\n");
        return 1;
      }

      if (dfs_extract_file(diskfile, acorn_filep, file)) {
        fclose(file);
        return 1;
      }

      fclose(file);
      acorn_filep++;
    }
  }

  printf("%d files extracted\n", acorn_dirp->num_of_files);

  fclose(diskfile);

  return 0;
}

static int format_diskfile(int argc, char * argv[]) {
  FILE * diskfile = NULL;

  if (argc < 2) {
    short_help();
    return 1;
  }

  if (strlen(argv[1]) > 12) {
    fprintf(stderr, "Diskname too long. Max 12 characters!");
    return 1;
  }

  printf("Writing: %s\n", argv[1]);
  diskfile = fopen(argv[0], "wb");
  if (diskfile == NULL) {
    perror("dfsutils");
    return 1;
  }

  if (dfs_format_diskfile(tracks * DFS_SECTORS_PER_TRACK, argv[1], diskfile)) {
    fclose(diskfile);
    return 1;
  }

  fclose(diskfile);
  return 0;
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
        exit(0);
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
        exit(1);
    }
  }

  argc -= optind;
  argv += optind;

  /* Check we're not trying to do two things at once */
  if (actions > 1) {
    help();
    exit(1);
  }

  if (argc < 1) {
    short_help();
    exit(1);
  }

  if (do_add) {
    return 0;
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
