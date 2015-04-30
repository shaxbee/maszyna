#include "parser.h"

#include <algorithm>
#include <iterator>
#include <cctype>

Parser::Parser(std::istream& input, const std::string delim):
    _input(input), _delim(delim), _location {0, 0} {
}

Parser::Token Parser::readToken() {
    if(_location.line == 0) {
        std::getline(_input, _line);
        _location.line = 1;
    };

    std::size_t start;
    std::size_t end;

    do {
        // ignore leading white space characters
        start = std::min(_line.find_first_not_of(" \t", _location.column), _line.size());

        // find end of token - either delimiter or end of line
        end = std::min(_line.find_first_of(_delim, start), _line.size());

        // no token was found - advance to next line
        if(start == end) {
            std::getline(_input, _line);
            _location.line++;
            _location.column = 0;
        }
    } while(start == end);

    return { start, end - start };
}

std::string Parser::readString() {
    auto token = readToken();
    auto result = _line.substr(token.pos, token.len);
    _location.column = token.pos + token.len + 1;

    return result;
}

template <typename T>
T Parser::checkAndAdvance(T val, const Token& token, char* end) {
    if(end - _line.c_str() < token.pos + token.len) { 
        throw ParseError("Unexpected trailing characters", _location);
    }
    _location.column = token.pos + token.len + 1;
    return val;
}

std::int32_t Parser::readInt32() {
    return static_cast<std::int32_t>(readInt64());
}

std::int64_t Parser::readInt64() {
    auto token = readToken();
    char* end = nullptr;
    return checkAndAdvance(std::strtoll(_line.c_str() + token.pos, &end, 10), token, end);
}

std::uint32_t Parser::readUInt32() {
    return static_cast<std::uint32_t>(readUInt64());
}

std::uint64_t Parser::readUInt64() {
    auto token = readToken();
    char* end = nullptr;
    return checkAndAdvance(std::strtoull(_line.c_str() + token.pos, &end, 10), token, end);
}

float Parser::readFloat() {
    auto token = readToken();
    char* end = nullptr;
    return checkAndAdvance(std::strtof(_line.c_str() + token.pos, &end), token, end); 
}

double Parser::readDouble() {
    auto token = readToken();
    char* end = nullptr;
    return checkAndAdvance(std::strtod(_line.c_str() + token.pos, &end), token, end); 
}

bool Parser::expectToken(const std::string expected) {
    return expectToken(expected.c_str());
}

bool Parser::expectToken(const char* expected) {
    auto token = readToken();
    auto result = _line.compare(token.pos, token.len, expected, token.len) == 0;
    _location.column = token.pos + token.len + 1;

    return result;
}
