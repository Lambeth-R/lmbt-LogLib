//
// Implementation of stream based logger.
// All main types are present.
// Works mainly with wchar_t data. Has option for extern Utf8 translation.
//

#pragma once
#include "../../Include/Common.h"
#include "../../Include/StringConvertLib.h"

#include <chrono>
#include <stdio.h>
#include <string>
#include <sstream>
#if defined PLATFORM_WIN
#include <Windows.h>
#endif

#if defined IS_CPP_20G
#include <format>
#endif

LIB_EXPORT
#if defined PLATFORM_WIN
std::wstringstream ParseLastWinError() noexcept;
#define PARSE_LAST_ERROR << ParseLastWinError()
#else
std::stringstream ParseLastPosixError() noexcept;
#define PARSE_LAST_ERROR << ParseLastPosixError()
#endif

struct LogLocation
{
    const wchar_t *file_path;
    const wchar_t *func_name;
    const wchar_t *line;

public:
    LIB_EXPORT
    explicit LogLocation(const wchar_t *fPath, const wchar_t *fName, const wchar_t *l) :
        file_path(fPath), func_name(fName), line(l) {};

    LIB_EXPORT
    std::wstring FormatInfo() const;
};

#ifdef LOG_FEATURE_LOCATION
#define LOG_LOCATION LogLocation(__FILEW__, __FUNCTIONW__, _CRT_WIDE(STRINGLIZE(__LINE__)))
#else
#define LOG_LOCATION
#endif

#ifdef USE_LOGGER
    #define LOGSTREAM(lvl, stream)      LogClass().Log(lvl) << LOG_LOCATION.FormatInfo() << L" " << stream
    #define LOGFORMAT(lvl, ...)         LogClass().Log(lvl) << LOG_LOCATION.FormatInfo() << L" " << LogClass::LogFormatWrap(__VA_ARGS__)
#if defined IS_CPP_20G
    #define LOGPRINT(lvl, format, ...)  LogClass().LogPrint(lvl, LOG_LOCATION, format, __VA_ARGS__)
#endif
#else
    #define LOGSTREAM()
    #define LOGPRINT()
    #define LOGFORMAT()
#endif // USE_LOGGER

#define Logstream_Error(str)       LOGSTREAM(LogClass::LogLevel_Error,   str)
#define Logstream_Warning(str)     LOGSTREAM(LogClass::LogLevel_Warning, str)
#define Logstream_Info(str)        LOGSTREAM(LogClass::LogLevel_Info,    str)
#define Logstream_Debug(str)       LOGSTREAM(LogClass::LogLevel_Debug,   str)

#define Log_Error(...)             LOGPRINT(LogClass::LogLevel_Error,    __VA_ARGS__)
#define Log_Warning(...)           LOGPRINT(LogClass::LogLevel_Warning,  __VA_ARGS__)
#define Log_Info(...)              LOGPRINT(LogClass::LogLevel_Info,     __VA_ARGS__)
#define Log_Debug(...)             LOGPRINT(LogClass::LogLevel_Debug,    __VA_ARGS__)

#if defined IS_CPP_20G
#define Log_ErrorF(...)            LOGFORMAT(LogClass::LogLevel_Error,    __VA_ARGS__)
#define Log_WarningF(...)          LOGFORMAT(LogClass::LogLevel_Warning,  __VA_ARGS__)
#define Log_InfoF(...)             LOGFORMAT(LogClass::LogLevel_Info,     __VA_ARGS__)
#define Log_DebugF(...)            LOGFORMAT(LogClass::LogLevel_Debug,    __VA_ARGS__)
#endif

struct ConsoleColor
{
    enum Color : uint8_t
    {
        Color_Black = 0,
        Color_Red,
        Color_Green,
        Color_Yellow,
        Color_Blue,
        Color_Magenta,
        Color_Cyan,
        Color_White,
        Color_NC    = 255,
    };
    static constexpr  Color         Lvl_to_Color(const uint8_t lvl);
    static const      std::wstring  SetColors(const std::wstring& iStream, const ConsoleColor::Color front, const ConsoleColor::Color back = Color_NC);
};

class LogClass
{
    static_assert(sizeof(wchar_t) == sizeof(uint16_t), "Use -fshort-wchar to enable small utf16 strings.");
public:
    enum LogLevel
    {
        LogLevel_None       = 0,
        LogLevel_Error      = 2,
        LogLevel_Warning    = 3,
        LogLevel_Info       = 4,
        LogLevel_Debug      = 5,
        LogLevel_All        = 10
    };

    enum LogType : uint16_t
    {
        LogType_None      = 0,
        LogType_Debug     = 2, // OutputDebugStringW
        LogType_Console   = 4, // stdout
        LogType_File      = 8, // Write to File
        LogType_Callback  = 16 // Callback to anythig u like via Log_Callback
    };

    typedef std::function<void (const std::wstring &message)> Log_Callback;

private:
    struct InnerStaticBlock
    {
        Log_Callback    Callback    = {};
        std::wstring    OutputFile  = {};
        LogType         LogFlags    = LogClass::LogType_None;
        LogLevel        CurLogLvl   = LogClass::LogLevel_None;
    };

    static InnerStaticBlock        &GetContext()
    {
        static InnerStaticBlock inner_block;
        return inner_block;
    }
           LogLevel                 m_LogLevel;
           std::wstringstream       m_Data;

#if defined PLATFORM_WIN
    static void                     SetConsoleUTF8(HANDLE hOut);

    static void                     InitConsole();
#endif

public:
    LIB_EXPORT
    static void                     InitLogger(LogLevel eLogLvl, LogType eFlags, const std::wstring &fName = {}, Log_Callback callback = {});
    LIB_EXPORT
    static void                     DumpPtr(const std::string &dumpName, void *ptr, size_t dumpSize);

    LIB_EXPORT
                                    LogClass(void) = default;
    LIB_EXPORT
                                    ~LogClass();

#if defined IS_CPP_20G
    template< class... Args >
    static std::string              LogFormatWrap(std::string_view format, Args&&... args)
    {
        return std::vformat(format, std::make_format_args(args...));
    }
    template< class... Args >
    static std::wstring             LogFormatWrap(std::wstring_view format, Args&&... args)
    {
        return std::vformat(format, std::make_wformat_args(args...));
    }
#endif
    LIB_EXPORT
    LogClass             &Log(const LogLevel elvl);
    LIB_EXPORT
    LogClass             &LogPrint(const LogLevel elvl, const LogLocation &locationInfo, const char *format, ...);
    LIB_EXPORT
    LogClass             &LogPrint(const LogLevel elvl, const LogLocation &locationInfo, const wchar_t *format, ...);

    LIB_EXPORT
    LogClass &operator<<(const std::wstring &data)
    {
        m_Data.write((wchar_t *) data.data(), data.size());
        return *this;
    }

    LIB_EXPORT
    LogClass &operator<<(const std::string &data)
    {
        return *this << ConvertUTF::UTF8_Decode(data);
    }

    LIB_EXPORT
    LogClass &operator<<(const wchar_t data)
    {
        std::wstring wTemp; wTemp.append(1, data);
        return *this << wTemp;
    }

    LIB_EXPORT
    LogClass &operator<<(const wchar_t *data)
    {
        return *this << std::wstring(data);
    }

    LIB_EXPORT
    LogClass &operator<<(const char data)
    {
        return *this << ConvertUTF::UTF8_Decode(std::string(&data,1));
    }

    LIB_EXPORT
    LogClass &operator<<(const char *data)
    {
        return *this << ConvertUTF::UTF8_Decode(data);
    }

    LIB_EXPORT
    LogClass &operator<<(const std::vector<uint8_t> &data)
    {
        m_Data.write((wchar_t *) data.data(), data.size());
        return *this;
    }

    LIB_EXPORT
    LogClass &operator<<(const LogLocation &location)
    {
        return *this << location.FormatInfo();
    }

    template <typename T>
    LogClass &operator<<(const T val)
    {
        m_Data << val;
        return *this;
    }

    template <typename T>
    LogClass &operator<<(const T &val)
    {
        m_Data << val;
        return *this;
    }

private:
    const wchar_t*                  GenerateAppendix() const;
};
DEFINE_BIT_OPS(LogClass::LogType, uint16_t);