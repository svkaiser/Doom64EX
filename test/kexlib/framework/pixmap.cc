
#include <gtest/gtest.h>

#include <framework/pixmap.h>

#define WIDTH 320
#define HEIGHT 240

TEST(Pixmap, create_and_free)
{
    Pixmap *pixmap;

    pixmap = Pixmap_New(WIDTH, HEIGHT, PF_RGB8);

    EXPECT_EQ(WIDTH, Pixmap_GetWidth(pixmap));
    EXPECT_EQ(HEIGHT, Pixmap_GetHeight(pixmap));

    Pixmap_Free(pixmap);
}