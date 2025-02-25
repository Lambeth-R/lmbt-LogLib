#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "../Include/Common.h"
#include "../Include/FsLib.h"
#include "../Include/LogLib.h"
#include "../Include/StringConvertLib.h"

#include <codecvt>
#include <iomanip>
#include <iostream>
#include <locale>

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

    setlocale(LC_ALL, ".65001");
    if (SetConsoleOutputCP(CP_UTF8))
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

void LogClass::InitLogger(LogLevel eLogLvl, LogType eFlags, const std::wstring &fName, Log_Callback callback)
{
#ifndef USE_LOGGER
    return;
#endif
    auto &logger_ctx = GetContext();
    if (eFlags & LogType_Print)
    {
        SetConsoleUTF8();
    }
    if (fName.empty() && eFlags & LogType_File)
    {
        eFlags ^= LogType_File;
    }
    if (eFlags & LogType_File)
    {
        std::wstringstream file_name;
        const auto &t = std::time(nullptr);
        const auto tm = std::localtime(&t);
        file_name << fName << std::put_time(tm, L"%Od_%Om") << L".log";
        logger_ctx.OutputFile = file_name.str();
        Fs_WriteFile(logger_ctx.OutputFile, std::vector<uint8_t>());
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
    const auto &t = std::time(nullptr);
    const auto tm = std::localtime(&t);
    file_name << dumpName << std::put_time(tm, "%Od_%Om") << ".bin";
    const auto casted_data = std::vector<uint8_t>(static_cast<uint8_t *>(ptr), static_cast<uint8_t *>(ptr) + dumpSize);
    Fs_WriteFile(file_name.str(), casted_data);
}

LogClass::~LogClass()
{
    const auto &logger_ctx = GetContext();

    if (m_LogLevel > logger_ctx.CurLogLvl)
    {
        return;
    }
    m_Data << std::endl;
    if (logger_ctx.LogFlags & LogType_Debug)
    {
        OutputDebugStringW(m_Data.str().c_str());
    }
    if (logger_ctx.LogFlags & LogType_Print)
    {
        std::wcout << ConsoleColor::SetColors(m_Data.str(), (uint16_t)ConsoleColor::Lvl_to_Color(m_LogLevel));
    }
    if (std::wcout.bad())
    {
        std::wcout.clear(0);
        fputwc(L'\n', stdout);
    }
    if (logger_ctx.LogFlags & LogType_File)
    {
        if (!Fs_AppendFile(logger_ctx.OutputFile, m_Data.str()))
        {
            OutputDebugStringW(L"Filed to produce output log into file.");
        }
    }
    if (logger_ctx.LogFlags & LogType_Callback && logger_ctx.Callback != nullptr)
    {
        logger_ctx.Callback(m_Data.str());
    }
}

std::wstringstream& LogClass::Log(LogClass::LogLevel elvl)
{
    m_LogLevel = elvl;
    const auto &t = std::time(nullptr);
    const auto tm = std::localtime(&t);
    m_Data << GenerateAppendix() << std::put_time(tm, L"%T");
    return m_Data;
}

std::wstringstream& LogClass::LogPrint(LogLevel elvl, const LogLocation &locationInfo, const wchar_t* format, ...)
{
    auto &wss = Log(elvl);
    wss << locationInfo.FormatInfo();
    va_list args_list;
    __crt_va_start(args_list, format);
    wss << L" " << string_format_l<wchar_t>(format, args_list);
    __crt_va_end(args_list);
    return wss;
}

#if _CPP_VERSION >= 201703L
std::wostream& operator<<(std::wostream& iStream, std::string_view iaString)
{
    const std::wstring& wTemp = ConvertUTF::UTF8_Decode(iaString);
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
    std::wstring wTemp = ConvertUTF::UTF8_Decode({ data }).c_str();
    iStream.write(wTemp.data(), wTemp.length());
    return iStream;
}

std::wostream& operator<<(std::wostream& iStream, const char data)
{
    std::string tString; tString.append(1, data);
    std::wstring wTemp = ConvertUTF::UTF8_Decode({ tString.data(), tString.length() }).c_str();
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

std::wostream& operator<<(std::wostream& iStream, const std::vector<uint8_t> &data)
{
    return iStream.write((wchar_t*)data.data(), data.size());
}

std::wostream& operator<<(std::wostream& iStream, const LogLocation &location)
{
    const auto &location_str = location.FormatInfo();
    return iStream.write(location_str.data(), location_str.size());
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
    case LogLevel_None:
    default:
        break;
    }
    return L"     ";
}

#pragma endregion