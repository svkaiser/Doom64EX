
#include <gtest/gtest.h>

#include <framework/Pixmap.hh>
#include <cstdint>

#define WIDTH 256
#define HEIGHT 128

static unique_ptr<uint8_t[]> rgb_data()
{
    uint8_t* data = new uint8_t[WIDTH * HEIGHT * 3];

    for (size_t y = 0; y < HEIGHT; y++)
    {
        for (size_t x = 0; x < WIDTH; x++)
        {
            uint8_t* ptr = &data[3 * (y * WIDTH + x)];

            ptr[0] = (uint8_t) y;
            ptr[1] = (uint8_t) x;
            ptr[2] = (uint8_t) (x ^ y);
        }
    }

    return unique_ptr<uint8_t[]>(data);
}

static Pixmap default_rgb_pixmap = Pixmap::FromData(rgb_data().get(), WIDTH, HEIGHT, PixelFormat::RGB);

TEST(Pixmap, FromDataRGB)
{
    EXPECT_EQ(WIDTH, default_rgb_pixmap.Width());
    EXPECT_EQ(HEIGHT, default_rgb_pixmap.GetHeight());
}

TEST(Pixmap, ScanlineIterator)
{
    size_t height = 0;

    for (auto line : default_rgb_pixmap)
    {
        height++;
    }

    EXPECT_EQ(HEIGHT, height);
}

TEST(Pixmap, PixelIterator)
{
    size_t height = 0;
    size_t width = 0;

    for (auto line : default_rgb_pixmap.Map<PixelFormat::RGB>())
    {
        for (auto px : line)
        {
            width++;
        }
        height++;
    }

    EXPECT_EQ(WIDTH, width);
    EXPECT_EQ(HEIGHT, height);
}