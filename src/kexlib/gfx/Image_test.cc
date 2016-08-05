
#include <gtest/gtest.h>

#include <kex/gfx/Image>

using namespace kex::gfx;

#define WIDTH 256
#define HEIGHT 128

namespace {

auto rgb_data()
{
    auto data = std::make_unique<uint8_t[]>(WIDTH * HEIGHT * 3);

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

    return data;
}

auto rgb_image = make_image(rgb_data().get(), pixel_format::rgb, WIDTH, HEIGHT);

}

TEST(Image, FromDataRgb)
{
    ASSERT_EQ(WIDTH, rgb_image.width());
    ASSERT_EQ(HEIGHT, rgb_image.height());
}

TEST(Image, ConvertRgbToRgba)
{
}
