#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include "../Include/Common.h"
#include "../Include/LogClass.h"
#include "../Include/UTF_Unocode.h"

#include <iostream>
#include <string>
#include <iomanip>
#include <codecvt>
#include <locale>

//In case we want this lib be inside .dll
#pragma data_seg(".STATIC")
std::wofstream      LogClass::s_OutputFile;
std::string         LogClass::s_Filename;
eLogType            LogClass::s_LogFlags = eLogType::None;
DWORD               LogClass::s_Error;
eLogLevels          LogClass::s_eCurLogLvl = eLogLevels::None;
bool                LogClass::s_ConsoleFixed = false;
#pragma data_seg()

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

void LogClass::InitLogger(eLogLevels eLogLvl, const std::string& fName, eLogType eFlags)
{
#ifndef USE_LOGGER
    return;
#endif
    s_eCurLogLvl = eLogLvl;
    s_LogFlags = eFlags;
    if ((DWORD)s_LogFlags & (DWORD)eLogType::Print)
    {
        SetConsoleUTF8();
    }
    if (fName.empty())
    {
        EraseFlag((DWORD)eLogType::File);
    }
    std::stringstream file_name;
    SYSTEMTIME sys_time;
    GetLocalTime(&sys_time);
    file_name << fName << string_format<char>("%02d_%02d.log", sys_time.wDay, sys_time.wMonth);
    s_Filename = file_name.str();
    s_OutputFile = std::wofstream(file_name.str(), std::ios::out | std::ios::binary);
}

bool LogClass::EraseFlag(DWORD flag)
{
    if ((DWORD)s_LogFlags & (DWORD)flag)
    {
        s_LogFlags = (eLogType)((DWORD)s_LogFlags ^ flag);
    }
    else 
    {
        return false;
    }
    return true;
}

const std::wstring LogClass::GenerateAppendix() const
{
    std::wstring result;
    switch (m_elogLevel)
    {
        case eLogLevels::None:
        {
            result.append(L"{None}");
            break;
        }
        case eLogLevels::Error:
        {
            result.append(L"{Error}");
            break;
        }
        case eLogLevels::Warning:
        {
            result.append(L"{Warn}");
            break;
        }
        case eLogLevels::Info:
        {
            result.append(L"{Info}");
            break;
        }
        case eLogLevels::Debug:
        {
            result.append(L"{Debug}");
            break;
        }
        case eLogLevels::All:
        {
            result.append(L"{All}");
            break;
        }
        default:
            break;
    }
    return result;
}

LogClass::~LogClass()
{
    if (m_elogLevel > s_eCurLogLvl)
    {
        return;
    }
    m_Data << std::endl;
    if ((DWORD)s_LogFlags & (DWORD)eLogType::Debug)
    {
        OutputDebugStringW(m_Data.str().c_str());
    }
    if ((DWORD)s_LogFlags & (DWORD)eLogType::Print)
    {
        std::wcout << ConsoleColour::SetColours(m_Data.str(), (uint16_t)ConsoleColour::Lvl_to_Colour(m_elogLevel));
    }
    if (std::wcout.bad())
    {
        std::wcout.clear(0);
        printf("\n");
    }
    if ((DWORD)s_LogFlags & (DWORD)eLogType::File)
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
}

std::wstringstream& LogClass::Log(eLogLevels elvl)
{
    m_elogLevel = elvl;
    SYSTEMTIME sys_time;
    GetLocalTime(&sys_time);
    m_Data << GenerateAppendix()<< L"\t" << string_format<wchar_t>(L"[%02d:%02d:%02d]", sys_time.wHour, sys_time.wMinute, sys_time.wSecond);
    return m_Data;
}

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
        s_ConsoleFixed = true;
    }
}

#if !_HAS_CXX17
std::wostream& operator<<(std::wostream& iStream, const char* data)
{
    std::wstring wTemp = UTF8_Decode({ data }).c_str();
    iStream.write(wTemp.data(), wTemp.length());
    return iStream;
}

std::wostream& operator<<(std::wostream& iStream, const char data)
{
    std::string tString; tString.append(1, data);
    std::wstring wTemp = UTF8_Decode({ tString.data(), tString.length()}).c_str();
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
#else
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