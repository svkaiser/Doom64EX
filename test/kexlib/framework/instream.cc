
#include <gtest/gtest.h>

#include <framework/instream.h>
#include <stdlib.h>

#define IDENT_STREAM_SIZE 256

static InStream *create_identity_memstream()
{
    int i;
    char *p;

    p = new char[IDENT_STREAM_SIZE];
    for (i = 0; i < IDENT_STREAM_SIZE; i++) {
        p[i] = (char) i;
    }

    return InStream_Mem(p, IDENT_STREAM_SIZE, K_TRUE);
}

TEST(InStream, TestMemNewAndFree)
{
    ISClose(create_identity_memstream());
}

TEST(InStream, TestMemSeek)
{
    InStream *is;

    is = create_identity_memstream();

    ISSeek(is, IDENT_STREAM_SIZE / 2, SEEK_SET);
    EXPECT_EQ(IDENT_STREAM_SIZE / 2, ISTell(is));

    ISClose(is);
}

TEST(InStream, TestMemRandomAccess)
{
    int i;
    size_t pos;
    InStream *is;

    is = create_identity_memstream();

    srand((unsigned int) time(NULL));
    for (i = 0; i < 10; i++) {
        char c = 0;

        pos = (size_t) (rand() % IDENT_STREAM_SIZE);

        ISSeek(is, pos, SEEK_SET);
        ISRead(is, &c, 1);

        EXPECT_EQ((char) pos, c);
    }

    ISClose(is);
}