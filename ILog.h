#pragma once

#include <memory> //std::make_unique
#include <cstdio> //std::snprintf

class ILog
{
private:
    /* data */
public:

template <typename... Args>
inline void log(const std::string &format, Args const &...args)
{
    const size_t size = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
    auto buf = std::make_unique<char[]>(size);
    std::snprintf(buf.get(), size, format.c_str(), args ...);
    // return std::string(buf.get());
}
    virtual ~ILog() = default;
};