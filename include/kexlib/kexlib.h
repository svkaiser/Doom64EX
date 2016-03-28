
#ifndef __DOOM64EX_KEXLIB_H__
#define __DOOM64EX_KEXLIB_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define KEXAPI extern

#ifdef __cplusplus
# define KEX_C_BEGIN extern "C" {
# define KEX_C_END }
#else
# define KEX_C_BEGIN
# define KEX_C_END
#endif

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) < (b)) ? (b) : (a))

#define K_BOOL int
#define K_TRUE 1
#define K_FALSE 0

#endif //__DOOM64EX_KEXLIB_H__
