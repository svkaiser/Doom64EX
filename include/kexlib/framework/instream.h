
#ifndef __DOOM64EX_KEXLIB_FRAMEWORK_INSTREAM_H__
#define __DOOM64EX_KEXLIB_FRAMEWORK_INSTREAM_H__

#include "../kexlib.h"
#include <stdio.h> // SEEK_*

KEX_C_BEGIN

struct InStream;
struct InStreamFn;

typedef struct InStream InStream;
typedef struct InStreamFn InStreamFn;

typedef size_t (*is_read_fn)(void *user, void *ptr, size_t size);
typedef size_t (*is_tell_fn)(void *user);
typedef size_t (*is_seek_fn)(void *user, long offset, int whence);
typedef void (*is_free_fn)(void *user);

struct InStreamFn {
    is_read_fn read;
    is_tell_fn tell;
    is_seek_fn seek;
    is_free_fn free;
};

KEXAPI InStream *InStream_New(void *user, const InStreamFn *fn);
KEXAPI InStream *InStream_File(const char *path);
KEXAPI InStream *InStream_Mem(const void *data, size_t size, K_BOOL free);

KEXAPI void *InStream_GetUserData(InStream *stream);
KEXAPI size_t InStream_Read(InStream *stream, void *ptr, size_t size);
KEXAPI size_t InStream_Tell(InStream *stream);
KEXAPI size_t InStream_Seek(InStream *stream, long offset, int whence);

KEXAPI void InStream_Free(InStream *ptr);

#define InStream_Close(stream) InStream_Free(stream);

#define ISUser(stream) InStream_GetUserData(stream)
#define ISRead(stream, ptr, size) InStream_Read(stream, ptr, size)
#define ISTell(stream) InStream_Tell(stream)
#define ISSeek(stream, offset, whence) InStream_Seek(stream, offset, whence)
#define ISClose(stream) InStream_Free(stream)

KEX_C_END

#endif //__DOOM64EX_KEXLIB_FRAMEWORK_INSTREAM_H__
