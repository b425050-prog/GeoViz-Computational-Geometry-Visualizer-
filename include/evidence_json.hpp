#pragma once
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
namespace gv {
inline std::string jsonText(const std::string &text) {
    std::ostringstream out;
    out << '"';
    for (unsigned char c : text) {
        if (c == '"' || c == '\\')
            out << '\\' << c;
        else if (c < 32)
            out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                << static_cast<unsigned>(c) << std::dec;
        else
            out << c;
    }
    out << '"';
    return out.str();
}
inline std::string consistencyHash(const std::string &bytes) {
    std::uint64_t value = 14695981039346656037ULL;
    for (unsigned char c : bytes) {
        value ^= c;
        value *= 1099511628211ULL;
    }
    std::ostringstream out;
    out << "fnv1a64:" << std::hex << std::setw(16) << std::setfill('0') << value;
    return out.str();
}
} // namespace gv
