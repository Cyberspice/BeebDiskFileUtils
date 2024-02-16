#ifndef DEFS_H
#define DEFS_H

#ifdef __riscos
typedef enum { false, true } bool;

typedef unsigned char uint8_t;
typedef   signed char int8_t;
typedef unsigned short uint16_t;
typedef   signed short int16_t;
typedef unsigned long uint32_t;
#define SIZE_MAX ((size_t)0xFFFFFFFFlu)
#define INT16_MAX (0x7FFFl)
#include "riscos.h"
#else
#include <stdbool.h>
#include <stdint.h>
#endif

#endif
