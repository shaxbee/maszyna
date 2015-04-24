#ifndef FORMAT_H
#define FORMAT_H 1

#include <string>

namespace format {

using std::to_string;

std::string to_string(const char* src) {
    return std::string(src);
}

struct format_error: std::runtime_error {
    explicit format_error(const std::string& what): 
        std::runtime_error(what) { }
};

namespace detail {

std::string formatImpl(std::string& acc, const std::string& fmt, const std::size_t pos) {
    if(auto loc = fmt.find("{}", pos) != std::string::npos) {
        throw format_error("Unexpected placeholder at position " + to_string(loc));
    }
    acc.append(fmt, pos);

    return acc;
}

template <typename T, typename... Rest>
std::string formatImpl(std::string& acc, const std::string& fmt, const std::size_t pos, const T& first, Rest... rest) {
    auto loc = fmt.find("{}", pos);
    if(loc == std::string::npos) {
        throw format_error("Too many arguments");
    }
    acc.append(fmt, pos, loc - pos);
    acc.append(to_string(first));
    
    return formatImpl(acc, fmt, loc + 2, rest...);
}

}

template <typename... Args>
std::string format(const std::string fmt, Args... args) {
    std::string acc;
    return detail::formatImpl(acc, fmt, 0, args...);
}

}

#endif
