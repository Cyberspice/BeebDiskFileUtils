# BeebDiskFileUtils

A collection of disk file utilities for manipulating DFS and ADFS disk image files for Acorn computers.

* dfsutils - For DFS disk images

## dfsutils

This is a utility that allows the manipulation of DFS disk image files. It can handle 40 and 80 track disk images. It does not currently handle non Acorn extensions to DFS.

The utility can:

* 'Format' a disk image file or 40 or 80 tracks
* Extract all the files from a disk image
* Extract specific files from a disk image
* Add a new file to a disk image
* Update a file's meta data in a disk image
* Remove a file from a disk image

The utility provides command line help using the -h option:

```
% ./dfsutils -h
dfsutils - Acorn DFS disk image utilities

Usage: dfsutils diskfile
   or: dfsutils --add [option] diskfile file load_address exec_address [locked]
   or: dfsutils --extract [option] diskfile [file [file]...]
   or: dfsutils --format [option] diskfile diskname
   or: dfsutils --remove [option] diskfile file [file [file]...]
   or: dfsutils --update [option] diskfile file load_address exec_address [locked]

Options:
       --40           Simulate 40 track disk
       --80           Simulate 80 track disk (default)
   -a, --add          Add a file to the disk image
   -d, --dir          Target directory
   -f, --format       Creates a disk image (overwrites any existing file)
   -h, --help         Display help
   -r, --remove       Remove a file from the disk image
   -u, --update       Update the properties of a file
   -v, --verbose      Raise the verbosity (can be used more than once)
   -x, --extract      Extract file(s)
```

### DFS file names

DFS file names are 7 characters together with a single character 'directory'. If a directory is not specified then the default of $ is assumed.  The DFS file is represented in the native operating system of the host as a 7 character file name with a single character extension comprising the 'directory' separated by a dot.  I.e. P.CODE becomes CODE.P

### Listing a DFS catalogue

To list a DFS catalogue from a disk image just specify the disk image file name as an argument

```
% ./dfsutils Acornsoft/Elite-MasterAndTubeEnhanced.ssd
Name   : ELITE128TUBE
Options: 3 (Exec)
----------------------------------------------------------------
  MOP.D            0x00005600 0x00005600       2560        691
  MOO.D            0x00005600 0x00005600       2560        681
  MON.D            0x00005600 0x00005600       2560        671
  MOM.D            0x00005600 0x00005600       2560        661
  MOL.D            0x00005600 0x00005600       2560        651
  MOK.D            0x00005600 0x00005600       2560        641
  MOJ.D            0x00005600 0x00005600       2560        631
  MOI.D            0x00005600 0x00005600       2560        621
  MOH.D            0x00005600 0x00005600       2560        611
  MOG.D            0x00005600 0x00005600       2560        601
  MOF.D            0x00005600 0x00005600       2560        591
  MOE.D            0x00005600 0x00005600       2560        581
  MOD.D            0x00005600 0x00005600       2560        571
  MOC.D            0x00005600 0x00005600       2560        561
  MOB.D            0x00005600 0x00005600       2560        551
  MOA.D            0x00005600 0x00005600       2560        541
  CODE.D           0x000011e3 0x000011e3      17437        472
  CODE.T           0x000011e3 0x000011e3      19997        393
  M128Elt          0xffff0e00 0xffff0e43        721        390
  ELITEa.I         0xffff2000 0xffff2000       5769        367
  CODE.I           0xffff2400 0xffff2c89       6454        341
  BDATA            0x00000000 0x00000000      16896        275
  ELPIC            0xffff0900 0xffff0900        362        273
  TubeElt          0xffff2000 0xffff2085        752        270
  BCODE            0x00000000 0x00000000      27720        161
  CODE.P           0x00001000 0x0000106a      38799          9
  A.E              0x00000000 0x00000000        256          8
  ELITE            0xffff1900 0xffff8023        403          6
  LOAD             0xffff1900 0xffff8023        270          4
  !BOOT            0x00000000 0xffffffff         14          3
  A1.E             0x00000000 0x00000000        256          2
----------------------------------------------------------------
31 files
```

### 'Formatting' a DFS disk image

To create a DFS disk image use the --format option. It takes two arguments, the disk image file name and a the DFS disk title.

The example below creates a '40 track' disk image. By default '80 track' disk images are created.

```
% ./dfsutils --40 --format melsdemo.ssd MELSDEMO
Writing: MELSDEMO
% ./dfsutils melsdemo.ssd
Name   : MELSDEMO
Options: 0 (None)
----------------------------------------------------------------
0 files
```

### Extracting files from a DFS disk image

To extract all the files from a DFS disk image use the --extract option. This will create a directory in the host file system with the name of the disk and extract all of the files in to it.

For example:

```
% ./dfsutils --extract Acornsoft/Elite-MasterAndTubeEnhanced.ssd
Output dir: ELITE128TUBE
Extracting: ELITE128TUBE/MOP.D
Extracting: ELITE128TUBE/MOO.D
Extracting: ELITE128TUBE/MON.D
Extracting: ELITE128TUBE/MOM.D
Extracting: ELITE128TUBE/MOL.D
Extracting: ELITE128TUBE/MOK.D
Extracting: ELITE128TUBE/MOJ.D
Extracting: ELITE128TUBE/MOI.D
Extracting: ELITE128TUBE/MOH.D
Extracting: ELITE128TUBE/MOG.D
Extracting: ELITE128TUBE/MOF.D
Extracting: ELITE128TUBE/MOE.D
Extracting: ELITE128TUBE/MOD.D
Extracting: ELITE128TUBE/MOC.D
Extracting: ELITE128TUBE/MOB.D
Extracting: ELITE128TUBE/MOA.D
Extracting: ELITE128TUBE/CODE.D
Extracting: ELITE128TUBE/CODE.T
Extracting: ELITE128TUBE/M128Elt
Extracting: ELITE128TUBE/ELITEa.I
Extracting: ELITE128TUBE/CODE.I
Extracting: ELITE128TUBE/BDATA
Extracting: ELITE128TUBE/ELPIC
Extracting: ELITE128TUBE/TubeElt
Extracting: ELITE128TUBE/BCODE
Extracting: ELITE128TUBE/CODE.P
Extracting: ELITE128TUBE/A.E
Extracting: ELITE128TUBE/ELITE
Extracting: ELITE128TUBE/LOAD
Extracting: ELITE128TUBE/!BOOT
Extracting: ELITE128TUBE/A1.E
31 files extracted
```

The output directory name can be overriden using the -d option.

One or more specific files can be specified after the disk image name and only those files will be extracted.

```
% ./dfsutils -d ELITE2 --extract Acornsoft/Elite-MasterAndTubeEnhanced.ssd TubeElt
Output dir: ELITE2
Extracting: ELITE2/TubeElt
1 files extracted
```

** Note that the file meta data such as load and execution addresses are lost **

### Adding files to a DFS disk image

A file can be added to a DFS disk image using the --add option.

```
% ./dfsutils --add melsdemo.ssd TubeElt 0xffff2000 0xffff2085
% ./dfsutils melsdemo.ssd
Name   : MELSDEMO
Options: 0 (None)
----------------------------------------------------------------
  TubeElt          0xffff2000 0xffff2085        752          3
----------------------------------------------------------------
1 files
```

The keyworked 'locked' can be optionally added as the last argument to lock the file.

** Note the file is added to the end of the files and there must be sufficient space on the disk for the file **

## Building

The utilities use [CMake](https://cmake.org).  To build...

```
% mkdir build
% cd build
% cmake ../CMakeLists.txt
% cd ..
% make
```

## Known issues

* When adding files it incorrectly counts the path as part of the file length
* File names are case sensisitive. They should be case preserving but insensitive.
