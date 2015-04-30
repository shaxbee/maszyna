#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <istream>
#include <tuple>

struct Location {
    std::size_t line;
    std::size_t column;
};

class ParseError: public std::runtime_error {
public:
    explicit ParseError(const std::string& what, const Location location):
        std::runtime_error(what), _location(location) { }
    virtual ~ParseError() { }

    const Location& location();

private:
    const Location _location;
};

class Parser
{
public:
    explicit Parser(std::istream& input, const std::string delim = " \t");

    const Location& location() const {
        return _location;
    }

    void ignoreToken();

    std::string readString();
    std::int32_t readInt32();
    std::uint32_t readUInt32();
    std::int64_t readInt64();
    std::uint64_t readUInt64();
    float readFloat();
    double readDouble();

    bool expectToken(const std::string expected);
    bool expectToken(const char* expected);

private:
    struct Token {
        std::size_t pos;
        std::size_t len;
    };

    Token readToken();
    
    template <typename T>
    T checkAndAdvance(T val, const Token& token, char* end);

    std::istream& _input;
    const std::string _delim;
    std::string _line;
    Location _location;
};

#endif
