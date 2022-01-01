#ifndef __DEBUG_H
#define __DEBUG_H

#define DEBUG_LEVEL_ERROR 1
#define DEBUG_LEVEL_WARNING 2
#define DEBUG_LEVEL_INFO 3
#define DEBUG_LEVEL_DEBUG 4

extern int verbosity;

#define DEBUG_LEVEL(L) (verbosity >= (L))

#endif