#include <string>
#include <sstream>
#include <functional>

#include "gtest/gtest.h"
#include "parser.h"

template <typename T>
T read(T (Parser::*fun)(), const std::string src) {
    auto stream = std::istringstream(src);
    auto parser = Parser(stream);
    return (parser.*fun)();
}

TEST(ParserTest, ReadValidToken) {
    EXPECT_EQ("foo", read(&Parser::readString, "foo bar"));
    EXPECT_EQ("foo", read(&Parser::readString, "  foo"));
    EXPECT_EQ(123, read(&Parser::readInt32, "123"));
    EXPECT_EQ(123u, read(&Parser::readUInt32, "123"));
    EXPECT_EQ(123ll, read(&Parser::readInt64, "123"));
    EXPECT_EQ(123ull, read(&Parser::readUInt64, "123"));
}

TEST(ParserTest, ReadMultipleTokens) {
    auto stream = std::istringstream("  foo 123  bar 2056.1 ");
    auto parser = Parser(stream);

    EXPECT_EQ("foo", parser.readString());
    EXPECT_EQ(123, parser.readInt32());
    EXPECT_EQ("bar", parser.readString());
    EXPECT_FLOAT_EQ(2056.1, parser.readFloat());
}
