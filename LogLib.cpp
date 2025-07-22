#include "../../Include/FsLib.h"
#include "../../Include/LogLib.h"

#include <cstring>
#include <time.h>
#include <codecvt>
#include <stdarg.h>
#include <iomanip>
#include <iostream>
#include <locale>

#if defined PLATFORM_WIN
extern std::wstring ForamtSystemMessage(const uint32_t messageId) noexcept;
#endif

std::wstring LogLocation::FormatInfo() const
{
    std::wstring result(file_path);
    auto fname_off = result.find_last_of(L'\\');
    if (fname_off != std::wstring::npos)
    {
        result = result.substr(fname_off + 1);
    }
    result += L"|";
    result += func_name;
    result += L":";
    result += line;
    return result;
}

#pragma region ConsoleColor

constexpr ConsoleColor::Color ConsoleColor::Lvl_to_Color(const uint8_t lvl)
{
    switch (lvl)
    {
    case LogClass::LogLevel_Error:
    case LogClass::LogLevel_Warning:
    case LogClass::LogLevel_Info:
    case LogClass::LogLevel_Debug:
        return static_cast<ConsoleColor::Color>(lvl);
        break;
    case LogClass::LogLevel_None:
    case LogClass::LogLevel_All:
    default:
        return ConsoleColor::ConsoleColor::Color_NC;
    }
}

const std::wstring ConsoleColor::SetColors(const std::wstring& iStream, const ConsoleColor::Color front, const ConsoleColor::Color back)
{
    if (front == -1)
    {
        return iStream;
    }
    std::wstringstream ws;
    ws << L"\033[";
    ws << std::dec << static_cast<uint32_t>(front + 30);
    if (back == Color_NC)
    {
        ws << L"m";
    }
    else
    {
        ws << L";" << std::dec << static_cast<uint32_t>(back + 40) << L"m";
    }

    ws << iStream << L"\033[0m";
    return ws.str();
}

#pragma endregion

#pragma region LogClass

#if defined PLATFORM_WIN
void LogClass::SetConsoleUTF8(HANDLE hOut)
{
    const DWORD console_mode = ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
          DWORD ret_console_mode = 0;

    if (GetConsoleOutputCP() != CP_UTF8)
    {
        SetConsoleOutputCP(CP_UTF8);
    }
    if (GetConsoleMode(hOut, &ret_console_mode) && console_mode != ret_console_mode)
    {
        ret_console_mode |= console_mode;
        SetConsoleMode(hOut, ret_console_mode);
    }
    std::locale loc(std::locale(), new std::codecvt<wchar_t, char, std::mbstate_t>(".65001"));
    std::wcout.imbue(loc);
    setlocale(LC_ALL, ".65001");
}

void LogClass::InitConsole()
{
    const DWORD last_err = GetLastError();
    DWORD this_err = 0;
    if (!AllocConsole())
    {
        this_err = GetLastError();
    }
    if (this_err != ERROR_ACCESS_DENIED)
    {
        FILE *cur_stdin = stdin, *cur_stdout = stdout, *cur_stderr = stderr;
        freopen_s(&cur_stdin,  "CONIN$", "r", stdin);
        freopen_s(&cur_stdout, "CONOUT$", "w", stdout);
        freopen_s(&cur_stderr, "CONOUT$", "w", stderr);
    }
    auto hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleUTF8(hOut);
    SetLastError(last_err);
}
#endif

void LogClass::InitLogger(LogLevel eLogLvl, LogType eFlags, const std::wstring &fName, Log_Callback callback)
{
#ifndef USE_LOGGER
    return;
#endif
    auto &logger_ctx = GetContext();
#if defined PLATFORM_WIN
    if (eFlags & LogType_Console)
    {
        InitConsole();
    }
#endif
    if (fName.empty() && eFlags & LogType_File)
    {
        eFlags ^= LogType_File;
    }
    if (eFlags & LogType_File)
    {
        std::wstringstream file_name;
        std::tm tm_data;
        const auto &time_data = std::time(nullptr);
#if defined PLATFORM_WIN
        ::localtime_s(&tm_data, &time_data);
#else
        ::localtime_r(&time_data, &tm_data);
#endif
        file_name << fName << std::put_time(&tm_data, L"%Od_%Om") << L".log";
        logger_ctx.OutputFile = file_name.str();
        fs::writeFile(logger_ctx.OutputFile, std::vector<uint8_t>());
    }
    if (eFlags & LogType_Callback)
    {
        logger_ctx.Callback = callback;
    }
    logger_ctx.CurLogLvl = eLogLvl;
    logger_ctx.LogFlags = eFlags;
}

void LogClass::DumpPtr(const std::string& dumpName, void* ptr, size_t dumpSize)
{
    std::stringstream file_name;
    std::tm tm_data;
    const auto &time_data = std::time(nullptr);
#if defined PLATFORM_WIN
    ::localtime_s(&tm_data, &time_data);
#else
    ::localtime_r(&time_data, &tm_data);
#endif
    file_name << dumpName << std::put_time(&tm_data, "%Od_%Om") << ".bin";
    const auto casted_data = std::vector<uint8_t>(static_cast<uint8_t *>(ptr), static_cast<uint8_t *>(ptr) + dumpSize);
    fs::writeFile(file_name.str(), casted_data);
}

LogClass::~LogClass()
{
    const auto &logger_ctx = GetContext();

    if (m_LogLevel > logger_ctx.CurLogLvl)
    {
        return;
    }
    m_Data.write(L"\n", 1);
#if defined PLATFORM_WIN
    if (logger_ctx.LogFlags & LogType_Debug)
    {
        OutputDebugStringW(m_Data.str().c_str());
    }
#endif
    if (logger_ctx.LogFlags & LogType_Console)
    {
        std::wcout << ConsoleColor::SetColors(m_Data.str(), ConsoleColor::Lvl_to_Color(m_LogLevel));
    }
    if (std::wcout.bad())
    {
        std::wcout.clear();
        fputwc(L'\n', stdout);
    }
    if (logger_ctx.LogFlags & LogType_File)
    {
        if (!fs::appendFile(logger_ctx.OutputFile, m_Data.str()))
        {
#if defined PLATFORM_WIN
            OutputDebugStringW(L"Filed to produce output log into file.");
#endif
        }
    }
    if (logger_ctx.LogFlags & LogType_Callback && logger_ctx.Callback != nullptr)
    {
        logger_ctx.Callback(m_Data.str());
    }
}

LogClass &LogClass::Log(const LogClass::LogLevel elvl)
{
    m_LogLevel = elvl;
    std::tm tm_data;
    const auto &time_data = std::time(nullptr);
#if defined PLATFORM_WIN
    ::localtime_s(&tm_data, &time_data);
#else
    ::localtime_r(&time_data, &tm_data);
#endif
    std::wstringstream wss;
    wss << std::put_time(&tm_data, L"%T");
    m_Data << GenerateAppendix() << wss.str();
    return *this;
}

LogClass &LogClass::LogPrint(const LogLevel elvl, const LogLocation &locationInfo, const char *format, ...)
{
    auto &wss = Log(elvl);
    wss << locationInfo.FormatInfo();
    va_list args_list;
    va_start(args_list, format);
    wss << L" ";
    wss << string_format_l<char>(format, args_list);
    va_end(args_list);
    return wss;
}

LogClass &LogClass::LogPrint(const LogLevel elvl, const LogLocation &locationInfo, const wchar_t* format, ...)
{
    auto &wss = Log(elvl);
    wss << locationInfo.FormatInfo();
    va_list args_list;
    va_start(args_list, format);
    wss << L" " << string_format_l<wchar_t>(format, args_list);
    va_end(args_list);
    return wss;
}

const wchar_t* LogClass::GenerateAppendix() const
{
    switch (m_LogLevel)
    {
        case LogLevel_Error:
        {
            return L"[Err]";
        }
        case LogLevel_Warning:
        {
            return L"[Wrn]";
        }
        case LogLevel_Info:
        {
            return L"[Inf]";
        }
        case LogLevel_Debug:
        {
            return L"[Dbg]";
        }
        case LogLevel_All:
        {
            return L">>>>>";
        }
        case LogLevel_Print:
        case LogLevel_None:
        default:
            break;
        }
    return L"";
}

#if defined PLATFORM_WIN
std::wstringstream ParseLastWinError() noexcept
{
    const auto last_error = GetLastError();
    std::wstringstream wss;
    wss << std::hex << last_error << L" " << ForamtSystemMessage(last_error);
    return wss;
}
#else
std::stringstream ParseLastPosixError() noexcept
{
    std::stringstream ss;
    ss << std::hex << errno << " " << std::strerror(errno);
    return ss;
}
#endif
#pragma endregion