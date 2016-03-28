#include "instream_priv.h"

InStream *InStream_New(void *user, const InStreamFn *fn)
{
    InStream *is;

    is = malloc(sizeof(*is));
    is->user = user;
    is->fn = fn;

    return is;
}

void InStream_Free(InStream *stream)
{
    if (stream) {
        if (stream->fn->free) {
            stream->fn->free(stream->user);
        }

        free(stream);
    }
}

void *InStream_GetUserData(InStream *stream)
{
    return stream->user;
}

size_t InStream_Read(InStream *stream, void *ptr, size_t size)
{
    return stream->fn->read(stream->user, ptr, size);
}

size_t InStream_Tell(InStream *stream)
{
    return stream->fn->tell(stream->user);
}

size_t InStream_Seek(InStream *stream, long offset, int whence)
{
    return stream->fn->seek(stream->user, offset, whence);
}