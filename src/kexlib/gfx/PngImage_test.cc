#include <kex/gfx/Image>
#include <fstream>
#include <gtest/gtest.h>

using namespace kex::gfx;

Rgb rgb_test_image[] = {
    { 0x00, 0x00, 0x00 }, { 0x7f, 0x7f, 0x7f }, { 0xff, 0xff, 0xff },
    { 0xff, 0x00, 0x00 }, { 0x00, 0xff, 0x00 }, { 0x00, 0x00, 0xff },
};

TEST(PngImage, LoadTest)
{
    std::ifstream file("./data/kexlib_test/rgb_test_image.png");
    ASSERT_TRUE(file.is_open());

    Image image(file);
}

TEST(PngImage, LoadIndexed)
{
    std::ifstream file("./data/kexlib_test/fire.png");
    ASSERT_TRUE(file.is_open());

    Image image(file);
}
