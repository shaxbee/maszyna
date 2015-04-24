#include "gtest/gtest.h"
#include "format.h"

TEST(FormatTest, Replacement) {
    EXPECT_EQ("3 apples", format::format("{} apples", 3));
    EXPECT_EQ("PI is 3.141592", format::format("PI is {}", 3.141592));
    EXPECT_EQ("Hello developer!", format::format("Hello {}!", "developer"));
    EXPECT_EQ("This test has 2 replacements", format::format("This {} has {} replacements", "test", 2));
}

TEST(FormatTest, NotEnoughArguments) {
    EXPECT_THROW(format::format("{} {}", 1), format::format_error);
}

TEST(FormatTest, TooManyArguments) {
    EXPECT_THROW(format::format("{}", 1, 2), format::format_error);
    EXPECT_THROW(format::format("", 1), format::format_error);
}
