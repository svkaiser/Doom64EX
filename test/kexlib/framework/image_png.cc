#include "gtest/gtest.h"

#include "framework/image.h"

TEST(Image_PNG, TestDetectFormat)
{
    InStream *stream;
    ImageFormat format;

    EXPECT_TRUE(stream = InStream_File("./data/kexlib_test/rgb_test_image.png"));

    format = Image_DetectFormat(stream);

    EXPECT_EQ(IF_PNG, format);
}