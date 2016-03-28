#include "instream_priv.h"

struct InStreamMem {
    const void *data;
    size_t size;
    size_t pos;
};

typedef struct InStreamMem InStreamMem;

static size_t InStreamFn_Mem_Read(InStreamMem *user, void *ptr, size_t size);

static size_t InStreamFn_Mem_Tell(InStreamMem *user);

static size_t InStreamFn_Mem_Seek(InStreamMem *user, long offset, int whence);

static const InStreamFn InStreamFn_Mem_With_Free = {
    (is_read_fn) InStreamFn_Mem_Read,
    (is_tell_fn) InStreamFn_Mem_Tell,
    (is_seek_fn) InStreamFn_Mem_Seek,
    free,
};

static const InStreamFn InStreamFn_Mem_Without_Free = {
    (is_read_fn) InStreamFn_Mem_Read,
    (is_tell_fn) InStreamFn_Mem_Tell,
    (is_seek_fn) InStreamFn_Mem_Seek,
    NULL,
};

InStream *InStream_Mem(const void *data, size_t size, K_BOOL free)
{
    InStreamMem *mem;

    mem = malloc(sizeof(*mem));
    mem->data = data;
    mem->size = size;
    mem->pos = 0;

    if (free) {
        return InStream_New(mem, &InStreamFn_Mem_With_Free);
    }
    else {
        return InStream_New(mem, &InStreamFn_Mem_Without_Free);
    }
}

static size_t InStreamFn_Mem_Read(InStreamMem *user, void *ptr, size_t size)
{
    if (user->pos + size >= user->size) {
        size = user->size - user->pos;
    }

    memcpy(ptr, user->data + user->pos, size);

    return size;
}

static size_t InStreamFn_Mem_Tell(InStreamMem *user)
{
    return user->pos;
}

static size_t InStreamFn_Mem_Seek(InStreamMem *user, long offset, int whence)
{
    if (whence == SEEK_CUR) {
        offset += user->pos;
        whence = SEEK_SET;
    }
    else if (whence == SEEK_END) {
        offset += user->size;
        whence = SEEK_SET;
    }

    if (whence == SEEK_SET) {
        if (offset < 0) {
            user->pos = 0;
        }
        else if (offset >= user->size) {
            user->pos = user->size;
        }
        else {
            user->pos = (size_t) offset;
        }
    }

    return user->pos;
}