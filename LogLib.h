//
// Implementation of stream based logger.
// All main types are present.
// Works mainly with wchar_t data. Has option for extern Utf8 translation.
//

#pragma once
#include "Common.h"

#include <stdio.h>
#include <string>
#include <sstream>
#include <Windows.h>

#if _CPP_VERSION >= 201703L
#define PARSE_ERROR(val) std::hex << val << " " << ForamtSystemMessage(val)
#else
#define PARSE_ERROR(val) std::hex << val << " " << ForamtSystemMessage(val).data()
#endif

LIB_EXPORT
std::wstring ParseLastError();

LIB_EXPORT
const std::wstring ForamtSystemMessage(const uint32_t messageId) noexcept;

struct LIB_EXPORT LogLocation
{
    const wchar_t *file_path;
    const wchar_t *func_name;
    const wchar_t *line;

    explicit LogLocation(const wchar_t *fPath, const wchar_t *fName, const wchar_t *l) :
        file_path(fPath), func_name(fName), line(l) {};

    std::wstring FormatInfo() const;
};

#ifdef LOG_FEATURE_LOCATION
#define LOG_LOCATION LogLocation(__FILEW__, __FUNCTIONW__, _CRT_WIDE(STRINGLIZE(__LINE__)))
#else
#define LOG_LOCATION
#endif


#ifdef USE_LOGGER
    #define LOGSTREAM(lvl, stream)      LogClass().Log(lvl) << LOG_LOCATION.FormatInfo() << L" " << stream
    #define LOGPRINT(lvl, format, ...)  LogClass().LogPrint(lvl, LOG_LOCATION, format, __VA_ARGS__)
#else
    #define LOGSTREAM()
    #define LOGPRINT()
#endif // USE_LOGGER

#define Logstream_Error(str)       LOGSTREAM(LogClass::LogLevel_Error,   str)
#define Logstream_Warning(str)     LOGSTREAM(LogClass::LogLevel_Warning, str)
#define Logstream_Info(str)        LOGSTREAM(LogClass::LogLevel_Info,    str)
#define Logstream_Debug(str)       LOGSTREAM(LogClass::LogLevel_Debug,   str)

#define Log_Error(...)     LOGPRINT(LogClass::LogLevel_Error,    __VA_ARGS__)
#define Log_Warning(...)   LOGPRINT(LogClass::LogLevel_Warning,  __VA_ARGS__)
#define Log_Info(...)      LOGPRINT(LogClass::LogLevel_Info,     __VA_ARGS__)
#define Log_Debug(...)     LOGPRINT(LogClass::LogLevel_Debug,    __VA_ARGS__)

struct ConsoleColor
{
    enum Color
    {
        Color_NC = -1,
        Color_Black,
        Color_Red,
        Color_Green,
        Color_Yellow,
        Color_Blue,
        Color_Magenta,
        Color_Cyan,
        Color_White,
    };
    static constexpr  Color         Lvl_to_Color(uint8_t lvl);
    static const      std::wstring  SetColors(const std::wstring& iStream, int16_t front, int16_t back = -1);
};

class LIB_EXPORT LogClass
{
public:
    enum LogLevel
    {
        LogLevel_None       = 0,
        LogLevel_Error      = 1,
        LogLevel_Warning    = 2,
        LogLevel_Info       = 3,
        LogLevel_Debug      = 4,
        LogLevel_All        = 10
    };

    enum LogType : uint16_t
    {
        LogType_None      = 0,
        LogType_Debug     = 2,
        LogType_Print     = 4,
        LogType_File      = 8,
        LogType_Callback  = 16
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

    static InnerStaticBlock &GetContext()
    {
        static InnerStaticBlock inner_block;
        return inner_block;
    }
           LogLevel                 m_LogLevel;
           std::wstringstream       m_Data;

    static void                     SetConsoleUTF8();
public:
    static void                     InitConsole();
    static void                     InitLogger(LogLevel eLogLvl, LogType eFlags, const std::wstring &fName = {}, Log_Callback callback = {});
    static void                     DumpPtr(const std::string &dumpName, void *ptr, size_t dumpSize);

                                    LogClass(void) = default;
                                    ~LogClass();

    std::wstringstream&             Log(LogLevel elvl);
    std::wstringstream&             LogPrint(LogLevel elvl, const LogLocation &locationInfo, const wchar_t *format, ...);
#if _HAS_CXX17
    LIB_EXPORT
    friend std::wostream&           operator<<(std::wostream &iStream, std::string_view  iString);
    LIB_EXPORT
    friend std::wostream&           operator<<(std::wostream &iStream, std::wstring_view iString);
#endif
    LIB_EXPORT
    friend std::wostream&           operator<<(std::wostream &iStream, const char *data);
    LIB_EXPORT
    friend std::wostream&           operator<<(std::wostream &iStream, const char data);
    LIB_EXPORT
    friend std::wostream&           operator<<(std::wostream &iStream, const wchar_t *data);
    LIB_EXPORT
    friend std::wostream&           operator<<(std::wostream &iStream, const wchar_t data);
    LIB_EXPORT
    friend std::wostream&           operator<<(std::wostream &iStream, const std::vector<uint8_t> &data);

    LIB_EXPORT
    friend std::wostream &operator<<(std::wostream &iStream, const LogLocation &location);

private:
    const wchar_t*                  GenerateAppendix() const;
};
DEFINE_BIT_OPS(LogClass::LogType, uint16_t);