
#include <gtest/gtest.h>

#include <framework/pixmap.h>

static Pixmap *windows_logo(int width, int height, int pitch)
{
    Pixmap *pixmap;
    int x, y;
    rgb8_t *line;

    if (!(pixmap = Pixmap_New(width, height, pitch, PF_RGB8, NULL)))
        return NULL;

    for (y = 0; y < height / 2; y++) {
        line = (rgb8_t *) Pixmap_GetScanline(pixmap, (size_t) y);

        for (x = 0; x < width / 2; x++) {
            line[x] = PixelRGB8_Red;
        }

        for (x = width / 2; x < width; x++) {
            line[x] = PixelRGB8_Green;
        }
    }

    for (y = height / 2; y < height; y++) {
        line = (rgb8_t *) Pixmap_GetScanline(pixmap, (size_t) y);

        for (x = 0; x < width / 2; x++) {
           line[x] = PixelRGB8_Blue;
        }

        for (x = width / 2; x < width; x++) {
           line[x] = PixelRGB8_Yellow;
        }
    }

    return pixmap;
}


TEST(Pixmap, TestNewAndSizeGetters)
{
    PixmapError error;
    Pixmap *pixmap;

    const int width = 320;
    const int height = 240;
    const int area = width * height;

    pixmap = Pixmap_New(width, height, 0, PF_RGB8, &error);

    EXPECT_EQ(width, Pixmap_GetWidth(pixmap));
    EXPECT_EQ(height, Pixmap_GetHeight(pixmap));
    EXPECT_EQ(area, Pixmap_GetArea(pixmap));
    EXPECT_EQ(3 * area, Pixmap_GetSize(pixmap)); // If broken, it's likely to do with pitch
    EXPECT_EQ(PIXMAP_ESUCCESS, error);

    Pixmap_Free(pixmap);
}

TEST(Pixmap, TestGetRGB8)
{
    Pixmap *pixmap;

    pixmap = windows_logo(200, 200, 0);

    EXPECT_EQ(PixelRGB8_Red, Pixmap_GetRGB8(pixmap, 50, 50));
    EXPECT_EQ(PixelRGB8_Green, Pixmap_GetRGB8(pixmap, 150, 50));
    EXPECT_EQ(PixelRGB8_Blue, Pixmap_GetRGB8(pixmap, 50, 150));
    EXPECT_EQ(PixelRGB8_Yellow, Pixmap_GetRGB8(pixmap, 150, 150));

    Pixmap_Free(pixmap);
}