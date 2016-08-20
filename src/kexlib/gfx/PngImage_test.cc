#include <kex/gfx/Image>
#include <fstream>
#include <gtest/gtest.h>

using namespace kex::gfx;

TEST(PngImage, load_rgb)
{
    std::ifstream file("testdata/color.png");
    ASSERT_TRUE(file.is_open());

    Image image(file);
    ASSERT_EQ(PixelFormat::rgb, image.format());
    ASSERT_EQ(320, image.width());
    ASSERT_EQ(240, image.height());
    ASSERT_EQ(nullptr, image.palette());
}

TEST(PngImage, load_rgba)
{
    std::ifstream file("testdata/color-alpha.png");
    ASSERT_TRUE(file.is_open());

    Image image(file);
    ASSERT_EQ(PixelFormat::rgba, image.format());
    ASSERT_EQ(320, image.width());
    ASSERT_EQ(240, image.height());
    ASSERT_EQ(nullptr, image.palette());
}

TEST(PngImage, load_index8_with_rgb_pal)
{
    std::ifstream file("testdata/index.png");
    ASSERT_TRUE(file.is_open());

    Image image(file);
    ASSERT_EQ(PixelFormat::index8, image.format());
    ASSERT_EQ(320, image.width());
    ASSERT_EQ(240, image.height());
    ASSERT_NE(nullptr, image.palette());
    ASSERT_EQ(PixelFormat::rgb, image.palette()->format());
}

TEST(PngImage, load_index8_with_rgba_pal)
{
    std::ifstream file("testdata/index-alpha.png");
    ASSERT_TRUE(file.is_open());

    Image image(file);
    ASSERT_EQ(PixelFormat::index8, image.format());
    ASSERT_EQ(320, image.width());
    ASSERT_EQ(240, image.height());
    ASSERT_NE(nullptr, image.palette());
    ASSERT_EQ(PixelFormat::rgba, image.palette()->format());
}

TEST(PngImage, resize_crop_rgb)
{
    std::ifstream file_expect("testdata/color-crop.png");
    std::ifstream file("testdata/color.png");
    ASSERT_TRUE(file_expect.is_open());
    ASSERT_TRUE(file.is_open());

    Image image_expect(file_expect);
    Image image(file);

    image.resize(image_expect.width(), image_expect.height());

    ASSERT_EQ(image_expect, image);
}
