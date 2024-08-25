#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#define MODULE_NAME L"Logger"

#include "../Include/Common.h"
#include "../Include/LogClass.h"
#include "../Include/UTF_Unocode.h"

#include <codecvt>
#include <iomanip>
#include <iostream>
#include <locale>
#include <string>

#if FEATURE_STATIC_LINKING == 1
#pragma data_seg(".STATIC")
#endif
LogClass::CallbackEx    LogClass::s_Callback        = {};
std::wofstream          LogClass::s_OutputFile      = {};
LogClass::LogLevel      LogClass::s_CurLogLvl       = LogLevel_None;
LogClass::LogType       LogClass::s_LogFlags        = LogType_None;
#if FEATURE_STATIC_LINKING == 1
#pragma data_seg()
#endif

std::wstring ParseLastError()
{
    const auto last_error = GetLastError();
    std::wstringstream wss;
    wss << PARSE_ERROR(last_error);
    return wss.str();
}

std::wstring ForamtSystemMessage(const uint32_t messageId) noexcept
{
    const DWORD curr_last_err = GetLastError();
    DWORD lang_id = 0;

    wchar_t* inner_allocated_buf = nullptr;
    MakeScopeGuard([=]() {
        if (inner_allocated_buf)
        {
            LocalFree(inner_allocated_buf);
        }});

    const int ret = GetLocaleInfoEx(
        LOCALE_NAME_SYSTEM_DEFAULT, 
        LOCALE_ILANGUAGE | LOCALE_RETURN_NUMBER,
        reinterpret_cast<LPWSTR>(&lang_id),
        sizeof(lang_id) / sizeof(wchar_t)
    );

    if (ret == 0)
    {
        lang_id = 0;
    }

    const DWORD ret_size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, messageId, lang_id, (wchar_t*) &inner_allocated_buf, 0, nullptr);
    SetLastError(curr_last_err);
    return std::wstring(inner_allocated_buf, static_cast<const size_t>(ret_size) - 2);
}

#pragma region ConsoleColor

constexpr ConsoleColor::Color ConsoleColor::Lvl_to_Color(uint8_t lvl)
{
    switch (lvl)
    {
    case LogClass::LogLevel_Error:
    case LogClass::LogLevel_Warning:
    case LogClass::LogLevel_Info:
    case LogClass::LogLevel_Debug:
        return static_cast<ConsoleColor::Color>((uint16_t)lvl);
        break;
    case LogClass::LogLevel_None:
    case LogClass::LogLevel_All:
    default:
        return ConsoleColor::ConsoleColor::Color_NC;
    }
}

const std::wstring ConsoleColor::SetColors(const std::wstring& iStream, int16_t front, int16_t back)
{
    if (front == -1)
    {
        return iStream;
    }
    std::wstringstream ws;
    static wchar_t fCode[20] = { 0 };
    static wchar_t bCode[20] = { 0 };
    static wchar_t cleanCode[] = L"\033[0m";
    if (back > 0)
    {
        swprintf_s(fCode, L"\033[%d;%dm", (uint16_t)front + 30, (uint16_t)back + 40);
    }
    else
    {
        swprintf_s(fCode, L"\033[%dm", (uint16_t)front + 30);
    }
    ws << fCode;
    ws << iStream;
    ws << cleanCode;
    return ws.str();
}

#pragma endregion

#pragma region LogClass

void LogClass::SetConsoleUTF8()
{
    std::locale loc(std::locale(), new std::codecvt<wchar_t, char, std::mbstate_t>(".65001"));
    std::wcout.imbue(loc);
    if (SetConsoleOutputCP(65001))
    {
        HANDLE handleOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD consoleMode;
        GetConsoleMode(handleOut, &consoleMode);
        consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        consoleMode |= DISABLE_NEWLINE_AUTO_RETURN;
        SetConsoleMode(handleOut, consoleMode);
        CloseHandle(handleOut);
    }
}

void LogClass::InitConsole()
{
    CONSOLE_SCREEN_BUFFER_INFO console_info;
    auto hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!GetConsoleScreenBufferInfo(hOut, &console_info))
    {
        AllocConsole();
        freopen_s((FILE**)stdin,  "CONIN$",  "r", stdin);
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
        freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
    }
    if (hOut && hOut != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hOut);
        hOut = nullptr;
    }
}

void LogClass::InitLogger(LogLevel eLogLvl, LogType eFlags, const std::string &fName, const CallbackEx &callback)
{
#ifndef USE_LOGGER
    return;
#endif
    if (eFlags & LogType_Print)
    {
        SetConsoleUTF8();
    }
    if (fName.empty() && eFlags & LogType_File)
    {
        eFlags ^= LogType_File;
    }
    if (eFlags & LogType_File && s_OutputFile.is_open())
    {
        s_OutputFile.close();
    }
    if (eFlags & LogType_File)
    {
        std::stringstream file_name;
        SYSTEMTIME sys_time;
        GetLocalTime(&sys_time);
        file_name << fName << string_format<char>("%02d_%02d.log", sys_time.wDay, sys_time.wMonth);
        s_OutputFile = std::wofstream(file_name.str(), std::ios::out | std::ios::binary);
    }
    if (eFlags & LogType_Callback)
    {
        s_Callback = callback;
    }
    s_CurLogLvl = eLogLvl;
    s_LogFlags = eFlags;
}

void LogClass::DumpPtr(const std::string& dumpName, void* ptr, size_t dumpSize)
{
    std::stringstream file_name;
    SYSTEMTIME sys_time;
    GetLocalTime(&sys_time);
    file_name << dumpName << string_format<char>("%02d_%02d.bin", sys_time.wDay, sys_time.wMonth);
    auto oStream = std::ofstream(file_name.str(), std::ios::out | std::ios::binary);
    oStream.write(static_cast<char*>(ptr), dumpSize);
    if (oStream.bad())
    {
        LOGPRINT(LogClass::LogLevel_Error, L"Failed to dump data");
    }
    oStream.flush();
    oStream.close();
}

LogClass::~LogClass()
{
    if (m_LogLevel > s_CurLogLvl)
    {
        return;
    }
    m_Data << std::endl;
    if (s_LogFlags & LogType_Debug)
    {
        OutputDebugStringW(m_Data.str().c_str());
    }
    if (s_LogFlags & LogType_Print)
    {
        std::wcout << ConsoleColor::SetColors(m_Data.str(), (uint16_t)ConsoleColor::Lvl_to_Color(m_LogLevel));
    }
    if (std::wcout.bad())
    {
        std::wcout.clear(0);
        fputwc(L'\n', stdout);
    }
    if (s_LogFlags & LogType_File)
    {
        if (s_OutputFile.is_open())
        {
            std::wstring tData(m_Data.str().c_str());
            s_OutputFile.write(tData.data(), tData.length());
            s_OutputFile.flush();
            if (s_OutputFile.bad())
            {
                OutputDebugStringW(L"Problem with file or/and locals.\n");
            }
        }
        else
        {
            OutputDebugStringW(L"Warning!! Logger is not initilized!");
        }
    }
    if (s_LogFlags & LogType_Callback && s_Callback.callback != nullptr)
    {
        (*s_Callback.callback)(s_Callback.callbackEx, m_Data.str());
    }
}

std::wstringstream& LogClass::Log(LogClass::LogLevel elvl)
{
    m_LogLevel = elvl;
    SYSTEMTIME sys_time;
    GetLocalTime(&sys_time);
    m_Data << GenerateAppendix() << L"\t" << string_format<wchar_t>(L"[%02d:%02d:%02d]", sys_time.wHour, sys_time.wMinute, sys_time.wSecond);
    return m_Data;
}

std::wstringstream& LogClass::LogPrint(LogLevel elvl, const wchar_t* moduleName, const wchar_t* format, ...)
{
    auto &wss = Log(elvl);
    wss << moduleName;
    va_list args_list;
    __crt_va_start(args_list, format);
    wss << L" " << string_format_l<wchar_t>(format, args_list);
    __crt_va_end(args_list);
    return wss;
}

#if _HAS_CXX17
std::wostream& operator<<(std::wostream& iStream, std::string_view iaString)
{
    const std::wstring& wTemp = UTF8_Decode(iaString);
    iStream.write(wTemp.data(), wTemp.length());
    return iStream;
}

std::wostream& operator<<(std::wostream& iStream, std::wstring_view iwString)
{
    iStream.write(iwString.data(), iwString.length());
    return iStream;
}
#endif

std::wostream& operator<<(std::wostream& iStream, const char* data)
{
    std::wstring wTemp = UTF8_Decode({ data }).c_str();
    iStream.write(wTemp.data(), wTemp.length());
    return iStream;
}

std::wostream& operator<<(std::wostream& iStream, const char data)
{
    std::string tString; tString.append(1, data);
    std::wstring wTemp = UTF8_Decode({ tString.data(), tString.length() }).c_str();
    iStream.write(wTemp.data(), wTemp.length());
    return iStream;
}

std::wostream& operator<<(std::wostream& iStream, const wchar_t* data)
{
    std::wstring wTemp(data);
    iStream.write(wTemp.data(), wTemp.length());
    return iStream;
}

std::wostream& operator<<(std::wostream& iStream, const wchar_t data)
{
    std::wstring wTemp; wTemp.append(1, data);
    iStream.write(wTemp.data(), wTemp.length());
    return iStream;
}

const wchar_t* LogClass::GenerateAppendix() const
{
    switch (m_LogLevel)
    {
    case LogLevel_Error:
    {
        return L"{Err}";
    }
    case LogLevel_Warning:
    {
        return L"{Wrn}";
    }
    case LogLevel_Info:
    {
        return L"{Inf}";
    }
    case LogLevel_Debug:
    {
        return L"{Dbg}";
    }
    case LogLevel_All:
    case LogLevel_None:
    default:
        break;
    }
    return L"";
}

#pragma endregion