#include "gtest/gtest.h"

#include "framework/image.h"
#include "../../../src/kexlib/framework/image/image.h"

static const char *png_path = "./data/kexlib_test/rgb_test_image.png";

TEST(Image_PNG, TestDetectFormat)
{
    InStream *stream;
    ImageFormat format;

    stream = InStream_File(png_path);
    format = Image_DetectFormat(stream);

    EXPECT_EQ(IF_PNG, format);

    ISClose(stream);
}

TEST(Image_PNG, TestLoadImage)
{
    InStream *stream;
    Image *image;

    stream = InStream_File(png_path);
    image = Image_Load(stream);

    EXPECT_EQ(3, Image_GetWidth(image));
    EXPECT_EQ(2, Image_GetHeight(image));

    Image_Free(image);
    ISClose(stream);
}