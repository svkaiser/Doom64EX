
#include <Kexdef>
#include <gtest/gtest.h>

int main(int argc, char *argv[])
{
    kex::init_lib();

    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}