
#include <gtest/gtest.h>

#include <framework/pixmap.h>

static void windows_logo(rgb8_t *data, int width, int height, int pitch)
{
    int x, y;
    rgb8_t *line;

    for (y = 0; y < height / 2; y++) {
        line = data + width * y;

        for (x = 0; x < width / 2; x++) {
            /* Top-left square is red */
            line[x] = PixelRGB8_Red;
        }

        for (x = width / 2; x < width; x++) {
            /* Top-right square is green */
            line[x] = PixelRGB8_Green;
        }
    }

    for (y = height / 2; y < height; y++) {
        line = data + width * y;

        for (x = 0; x < width / 2; x++) {
            /* Bottom-left square is blue */
            line[x] = PixelRGB8_Blue;
        }

        for (x = width / 2; x < width; x++) {
            /* Bottom-right square is yellow */
            line[x] = PixelRGB8_Yellow;
        }
    }
}

static Pixmap *windows_logo_pixmap_for_pitch()
{
    PixmapError error;
    rgb8_t *data;
    Pixmap *pixmap;

    const int width = 198;
    const int width_padded = 200;
    const int height = 200;
    const int pitch = 8;

    data = new rgb8_t[width_padded * height];

    windows_logo(data, width_padded, height, pitch);

    pixmap = Pixmap_NewFrom(data, width, height, pitch, PF_RGB8, &error);
    EXPECT_FALSE(error);

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
    EXPECT_FALSE(error);

    EXPECT_EQ(width, Pixmap_GetWidth(pixmap));
    EXPECT_EQ(height, Pixmap_GetHeight(pixmap));
    EXPECT_EQ(area, Pixmap_GetArea(pixmap));
    EXPECT_EQ(3 * area, Pixmap_GetSize(pixmap)); // If broken, it's likely to do with pitch

    Pixmap_Free(pixmap);
}

TEST(Pixmap, TestGetRGB8)
{
    PixmapError error;
    rgb8_t *data;
    Pixmap *pixmap;

    const int width = 200;
    const int height = 200;
    const int pitch = 0;

    data = new rgb8_t[width * height];

    windows_logo(data, width, height, 0);

    pixmap = Pixmap_NewFrom(data, width, height, pitch, PF_RGB8, &error);
    EXPECT_FALSE(error);

    EXPECT_EQ(PixelRGB8_Red, Pixmap_GetRGB8(pixmap, 50, 50));
    EXPECT_EQ(PixelRGB8_Green, Pixmap_GetRGB8(pixmap, 150, 50));
    EXPECT_EQ(PixelRGB8_Blue, Pixmap_GetRGB8(pixmap, 50, 150));
    EXPECT_EQ(PixelRGB8_Yellow, Pixmap_GetRGB8(pixmap, 150, 150));

    Pixmap_Free(pixmap);
    delete[] data;
}

TEST(Pixmap, TestGetScanlinePitch)
{
    Pixmap *pixmap;

    pixmap = windows_logo_pixmap_for_pitch();

    EXPECT_EQ(PixelRGB8_Red, ((rgb8_t*)Pixmap_GetScanline(pixmap, 99))[0]);
    EXPECT_EQ(PixelRGB8_Blue, ((rgb8_t*)Pixmap_GetScanline(pixmap, 100))[0]);
}

TEST(Pixmap, TestGetRGB8Pitch)
{
    Pixmap *pixmap;

    pixmap = windows_logo_pixmap_for_pitch();

    EXPECT_EQ(PixelRGB8_Green, Pixmap_GetRGB8(pixmap, 197, 99));
    EXPECT_EQ(PixelRGB8_Blue, Pixmap_GetRGB8(pixmap, 0, 100));
}