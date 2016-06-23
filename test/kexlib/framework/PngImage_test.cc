
#include <fstream>
#include <gtest/gtest.h>
#include <framework/Image>

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
    Pixmap_sp pixmap = image.data();

    Rgb *p = rgb_test_image;
    for (auto pixel : pixmap->map<Rgb>())
    {
        ASSERT_EQ(*p++, *pixel);
    }
}