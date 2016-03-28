#include "instream_priv.h"

/* The signature of `fread` is different from is_read_fn */
static size_t InStreamFn_File_Read(void *user, void *ptr, size_t size);

static const InStreamFn InStreamFn_File = {
    InStreamFn_File_Read,
    (is_tell_fn) ftell,
    (is_seek_fn) fseek,
    (is_free_fn) fclose,
};

InStream *InStream_File(const char *path)
{
    FILE *fd;

    if (!(fd = fopen(path, "rb"))) {
        return NULL;
    }

    return InStream_New(fd, &InStreamFn_File);
}

static size_t InStreamFn_File_Read(void *user, void *ptr, size_t size)
{
    return fread(ptr, 1, size, user);
}