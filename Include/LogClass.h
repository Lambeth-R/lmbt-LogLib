#pragma once
#include <sstream>
#include <fstream>
#include <thread>
#include <Windows.h>
#include <system_error>

#define PARSE_ERROR(val) std::hex << val << " " << std::system_category().message(val)

#ifdef USE_LOGGER
    #ifdef MODULE_NAME
        #define LOG(lvl, stream) LogClass().Log(lvl) << "(" << MODULE_NAME<< ")\t" << stream
    #else
        #define LOG(lvl, stream) LogClass().Log(lvl) << stream
    #endif

#else
    #define LOG()
#endif // _USE_LOGGER

enum class eLogType
{
    None = 0,
    Debug = 2,
    Print = 4,
    File = 8
};
DEFINE_ENUM_FLAG_OPERATORS(eLogType);

enum class eLogLevels
{
    None = 0,
    Error = 1,
    Warning = 2,
    Info = 3,
    Debug = 4,
    All = 10
};

class ConsoleColour
{
    enum class eConsoleColour
    {
        NC = -1,
        Black,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White,
    };
public:
    static constexpr eConsoleColour Lvl_to_Colour(eLogLevels lvl)
    {
        switch (lvl)
        {
        case eLogLevels::Error:
        case eLogLevels::Warning:
        case eLogLevels::Info:
        case eLogLevels::Debug:
            return eConsoleColour((uint16_t)lvl);
            break;
        case eLogLevels::None:
        case eLogLevels::All:
        default:
            return eConsoleColour::NC;
        }
    }
    static const std::wstring SetColours(const std::wstring& iStream, int16_t front, int16_t back = -1)
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
};



class LogClass
{
    #pragma region Static

    static eLogLevels       s_eCurLogLvl;
    static std::wofstream   s_OutputFile;
    static eLogType         s_LogFlags;
    static DWORD            s_Error;
    static std::string      s_Filename;
    static bool             s_ConsoleFixed;
    static bool EraseFlag(DWORD flag);
    static void SetConsoleUTF8();
public:
    static void InitLogger(eLogLevels eLogLvl, const std::string& fName = "", eLogType eFlags = eLogType::Debug);
    #pragma endregion

    #pragma region Public
    LogClass() = default;
    ~LogClass();
    std::wstringstream& Log(eLogLevels elvl);
    #pragma endregion

    #pragma region Private
private:
    
    const std::wstring GenerateAppendix() const;
    eLogLevels          m_elogLevel;
    std::wstringstream  m_Data;
    
    #pragma endregion
};

#if !_HAS_CXX17
std::wostream& operator<<(std::wostream& iStream, const char* data);
std::wostream& operator<<(std::wostream& iStream, const char data);
std::wostream& operator<<(std::wostream& iStream, const wchar_t* data);
std::wostream& operator<<(std::wostream& iStream, const wchar_t data);
#else
std::wostream& operator<<(std::wostream& iStream, const std::string_view iString);
std::wostream& operator<<(std::wostream& iStream, const std::wstring_view iString);
#endif