#pragma once
#include <string>
#include <memory>

#define USE_LOGGER
#include "LogClass.h"

template<typename ... Args>
const std::string string_format(const std::string &format, Args ... args)
{
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1;
    if (size_s <= 0) return {};
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args ...);
    return std::string(buf.get(), buf.get() + size - 1);
}

template<class T>
void SafeDelete(T **ptr)
{
    if (*ptr)
    {
        delete *ptr;
        *ptr = nullptr;
    }
}