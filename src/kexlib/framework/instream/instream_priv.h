#ifndef DOOM64EX_INSTREAM_PRIV_H
#define DOOM64EX_INSTREAM_PRIV_H

#include <framework/instream.h>

struct InStream {
    void *user;
    const InStreamFn *fn;
};

#endif //DOOM64EX_INSTREAM_PRIV_H
