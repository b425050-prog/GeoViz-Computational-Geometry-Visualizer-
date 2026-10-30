#pragma once
#include <charconv>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
namespace gv {
inline std::string exactNumber(double value) {
    char buffer[128];
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    if (result.ec != std::errc{})
        throw std::runtime_error("Cannot format numeric value.");
    return {buffer, result.ptr};
}
inline std::string utcText(std::int64_t epoch) {
    std::time_t value = static_cast<std::time_t>(epoch);
    std::tm result{};
#ifdef _WIN32
    if (gmtime_s(&result, &value))
        return std::to_string(epoch);
#else
    if (!gmtime_r(&value, &result))
        return std::to_string(epoch);
#endif
    if (result.tm_year + 1900 > 9999 || result.tm_year + 1900 < 1970)
        return std::to_string(epoch);
    std::ostringstream out;
    out << std::put_time(&result, "%Y-%m-%d %H:%M:%S");
    return out.str();
}
inline std::int64_t utcEpoch(const std::string &value) {
    if (value.find('-') == std::string::npos) {
        std::size_t n = 0;
        auto t = std::stoll(value, &n);
        if (n != value.size())
            throw std::runtime_error("Expected epoch seconds or YYYY-MM-DD HH:MM:SS UTC.");
        return t;
    }
    std::tm calendar{};
    std::istringstream in(value);
    in >> std::get_time(&calendar, "%Y-%m-%d %H:%M:%S");
    if (in.fail() || in.peek() != std::char_traits<char>::eof())
        throw std::runtime_error("Use YYYY-MM-DD HH:MM:SS UTC.");
#ifdef _WIN32
    auto t = _mkgmtime64(&calendar);
#else
    auto t = timegm(&calendar);
#endif
    if (t < 0 || utcText(t) != value)
        throw std::runtime_error("Invalid UTC calendar date/time.");
    return t;
}
} // namespace gv
