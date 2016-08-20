
#include <gtest/gtest.h>
#include <kex/gfx/Image>
#include <fstream>

using namespace kex::gfx;

template <class Callable>
Image create_rgb_image(uint16 width, uint16 height, Callable &func)
{
    Image image(PixelFormat::rgb, width, height, nullptr);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            Rgb c = func(x, y);
            image.set_pixel(x, y, c);
        }
    }

    return image;
};

TEST(Image, comparison)
{
    auto func_1 = [](int x, int y) { return Rgb(x, y, x ^ y); };
    auto img_1 = create_rgb_image(6, 8, func_1);
    auto img_2 = create_rgb_image(6, 8, func_1);

    ASSERT_TRUE(img_1 == img_2);
    ASSERT_FALSE(img_1 != img_2);

    auto func_2 = [](int x, int y) { return Rgb(y, x, x ^ y); };
    auto img_3 = create_rgb_image(6, 8, func_2);

    ASSERT_TRUE(img_1 != img_3);
    ASSERT_FALSE(img_1 == img_3);
}

TEST(Image, crop)
{
    std::ifstream fileExpect("testdata/color-crop.png");
    std::ifstream fileActual("testdata/color.png");
    ASSERT_TRUE(fileExpect.is_open());
    ASSERT_TRUE(fileActual.is_open());

    Image imageExpect(fileExpect);
    Image imageActual(fileActual);

    imageActual.resize(imageExpect.width(), imageExpect.height());

    ASSERT_EQ(imageExpect, imageActual);
}

TEST(Image, expand)
{
    std::ifstream fileExpect("testdata/color-expand.png");
    std::ifstream fileActual("testdata/color.png");
    ASSERT_TRUE(fileExpect.is_open());
    ASSERT_TRUE(fileActual.is_open());

    Image imageExpect(fileExpect);
    Image imageActual(fileActual);

    imageActual.resize(imageExpect.width(), imageExpect.height());

    ASSERT_EQ(imageExpect, imageActual);
}