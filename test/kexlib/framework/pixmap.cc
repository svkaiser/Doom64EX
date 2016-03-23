
#include <gtest/gtest.h>

#include <framework/pixmap.h>

TEST(pixmap, Pixmap_Return42_should_return_42)
{
    EXPECT_EQ(42, Pixmap_Return42());
}