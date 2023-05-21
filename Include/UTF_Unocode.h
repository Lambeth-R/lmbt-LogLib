#pragma once
#include <clocale>
#include <codecvt>
#include <string>
#include <string_view>
#include <algorithm>
#include <Windows.h>
#include <locale.h>


inline const std::wstring UTF8_Decode(const std::string_view aStr)
{
    const size_t& wstr_len = aStr.size() + 1;
    std::unique_ptr<wchar_t[]> twPtr(new wchar_t[wstr_len]);
    if (MultiByteToWideChar(CP_UTF8, 0, aStr.data(), -1, twPtr.get(), wstr_len) < wstr_len)
    {
        return {};
    }
    return std::wstring(twPtr.get(), wstr_len - 1);
}

inline const std::string UTF8_Encode(const std::wstring_view wStr)
{
    const size_t& astr_len = wStr.size() + 1;
    std::unique_ptr<char[]> taPtr(new char[astr_len]);
    if(WideCharToMultiByte(CP_ACP, 0, wStr.data(), -1, taPtr.get(), astr_len, 0, 0) < astr_len)
    {
        return {};
    };
    return std::string(taPtr.get(), astr_len - 1);
}