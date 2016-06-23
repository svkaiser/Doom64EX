
#include <gtest/gtest.h>

#include <framework/Pixmap>
#include <cstdint>

using namespace kex::gfx;

#define WIDTH 256
#define HEIGHT 128

namespace {

unique_ptr<uint8_t[]> rgb_data()
{
    uint8_t *data = new uint8_t[WIDTH * HEIGHT * 3];

    for (size_t y = 0; y < HEIGHT; y++)
    {
        for (size_t x = 0; x < WIDTH; x++)
        {
            uint8_t *ptr = &data[3 * (y * WIDTH + x)];

            ptr[0] = (uint8_t) y;
            ptr[1] = (uint8_t) x;
            ptr[2] = (uint8_t) (x ^ y);
        }
    }

    return unique_ptr<uint8_t[]>(data);
}

unique_ptr<uint8_t[]> index8_data()
{
    unique_ptr<uint8_t[]> data(new uint8_t[WIDTH * HEIGHT]);

    for (size_t y = 0; y < HEIGHT; y++)
    {
        for (size_t x = 0; x < WIDTH; x++)
        {
            data[y * WIDTH + x] = x ^ y;
        }
    }
}

Pixmap default_rgb_pixmap = Pixmap::from_data(rgb_data().get(), WIDTH, HEIGHT, PixelFormat::rgb);
//Pixmap default_index8_pixmap = Pixmap::from_data(index8_data().get(), WIDTH, HEIGHT, PixelFormat::index8);

}

TEST(Pixmap, FromDataRGB)
{
    ASSERT_EQ(WIDTH, default_rgb_pixmap.width());
    ASSERT_EQ(HEIGHT, default_rgb_pixmap.height());
}

TEST(Pixmap, PixelIteratorCount)
{
    size_t pixels = 0;

    for (auto pixel : default_rgb_pixmap.map<Rgb>())
        pixels++;

    ASSERT_EQ(WIDTH * HEIGHT, pixels);
}

TEST(Pixmap, PixelIteratorReadAccess)
{
    unique_ptr<uint8_t[]> data = rgb_data();
    size_t pxIndex = 0;

    for (auto pixel : default_rgb_pixmap.map<Rgb>())
    {
        Rgb rgb = *pixel;

        ASSERT_EQ(data[pxIndex * 3 + 0], rgb.red);
        ASSERT_EQ(data[pxIndex * 3 + 1], rgb.green);
        ASSERT_EQ(data[pxIndex * 3 + 2], rgb.blue);

        pxIndex++;
    }
}

TEST(Pixmap, PixelIteratorWriteAccess)
{
    Pixmap pixmap(640, 480, PixelFormat::rgb);
    size_t pxIndex = 0;

    // Set the pixels
    for (auto pixel : pixmap.map<Rgb>())
    {
        Rgb toSet{(uint8_t) (pxIndex / WIDTH), (uint8_t) (pxIndex / HEIGHT),
                  (uint8_t) ((pxIndex / WIDTH) ^ (pxIndex / HEIGHT))};

        pixel = toSet;

        pxIndex++;
    }

    // Verify that they're correctly set
    pxIndex = 0;
    for (auto pixel : pixmap.map<Rgb>())
    {
        Rgb expect{(uint8_t) (pxIndex / WIDTH), (uint8_t) (pxIndex / HEIGHT),
                   (uint8_t) ((pxIndex / WIDTH) ^ (pxIndex / HEIGHT))};

        Rgb actual = *pixel;

        ASSERT_EQ(expect.red, actual.red);
        ASSERT_EQ(expect.green, actual.green);
        ASSERT_EQ(expect.blue, actual.blue);

        pxIndex++;
    }
}

TEST(Pixmap, PixelMapIncorrectFormatException)
{
    ASSERT_ANY_THROW(default_rgb_pixmap.map<Rgba>());
}

TEST(Pixmap, ConvertRgbToRgba)
{
    Pixmap rgba = move(default_rgb_pixmap.convert(PixelFormat::rgba));
    size_t pxIndex = 0;

    for (auto pixel : rgba.map<Rgba>())
    {
        Rgba expect{(uint8_t) (pxIndex / WIDTH), (uint8_t) (pxIndex / HEIGHT),
                   (uint8_t) ((pxIndex / WIDTH) ^ (pxIndex / HEIGHT)), 0xff};

        Rgba actual = *pixel;

        ASSERT_EQ(expect.red, actual.red);
        ASSERT_EQ(expect.green, actual.green);
        ASSERT_EQ(expect.blue, actual.blue);
        ASSERT_EQ(expect.alpha, actual.alpha);

        pxIndex++;
    }
}