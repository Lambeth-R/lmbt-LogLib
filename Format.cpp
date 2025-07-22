#pragma once
#include "../../Include/Common.h"
#include "LogLib.h"
#if defined _MSC_VER

#include <windows.h>

#include <asferr.h>
#include <ddraw.h>

static HMODULE ParseError_FACILITY_ITF()
{
    return nullptr;
}

static HMODULE ParseError_FACILITY_NS()
{
    return nullptr;
}

static const std::wstring ForamtMessage(const uint32_t messageId, const DWORD langId)
{
    wchar_t *inner_allocated_buf = nullptr;
    MakeScopeGuard([=]() {if (inner_allocated_buf) { LocalFree(inner_allocated_buf); }});

    DWORD flag = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS;
    HMODULE sourceMdl = nullptr;
    MakeScopeGuard([=]() {if (sourceMdl) { FreeLibrary(sourceMdl); }});
    if ((messageId <= 0x1F3) ||
        (messageId >= 0x1F4 && messageId <= 0x3E7) ||
        (messageId >= 0x3E8 && messageId <= 0x513) ||
        (messageId >= 0x514 && messageId <= 0x6A3) ||
        (messageId >= 0x6A4 && messageId <= 0xF9F) ||
        (messageId >= 0xFA0 && messageId <= 0x176F) ||
        (messageId >= 0x1770 && messageId <= 0x2007) ||
        (messageId >= 0x2008 && messageId <= 0x2327) ||
        (messageId >= 0x2328 && messageId <= 0x2EDF) ||
        (messageId >= 0x2EE0 && messageId <= 0x3E7F))
    {
        flag |= FORMAT_MESSAGE_FROM_SYSTEM;
    }
    else if (messageId & (1 << 31) || messageId & (1 << 30)) // HRESULT error
    {
        flag |= FORMAT_MESSAGE_FROM_HMODULE;
        //if (messageId & (4 << 16)) // activprof.h
        // Multiple Sources.. >_<
        // FU Microsoft
        // Blank errores you can't do jack shit about, w/o context: 
        //      activprof.h, bthdef.h, cfgmgr32.h

        switch ((messageId >> 16) & 0xFF)
        {
            case 0x4:
            {
                // Parse FACILITY_ITF
                sourceMdl = ParseError_FACILITY_ITF();
            }
            case 0x7:
            {
                // Parse FACILITY_WIN32
            }
            case 0x876:
            {
                // Parse _FACDD
            }
            case 0xD:
            {
                sourceMdl = ParseError_FACILITY_NS();
            }
            case 0x879:
            case 0x880:
            {
                sourceMdl = LoadLibraryW(L"D3D10.dll");
                break;
            }
        }


        if (messageId & (0xD << 16))
        {
            if ((messageId & 0xFFFF) >= 0x7D0 && (messageId & 0xFFFF) < 0x80E) // asferr.h
            {
                sourceMdl = LoadLibraryW(L"asferror.dll");
            }
        }
        else if ((messageId & (4 << 16)) && (messageId & 0xFFFF) == 0x0600)
        {
            // ctffunc.h 
            // fherrors.h
            //sourceMdl = LoadLibraryW(L"ctffunc.dll");
        }
        else if ((messageId & (4 << 16)) && (messageId & 0xFFFF) >= 0x1000 && (messageId & 0xFFFF) < 0x1003)
        {
            // daogetrw.h
            // intshcut.h
            // reconcil.h
            // synchronizationerrors.h
            //sourceMdl = LoadLibraryW(L"daogetrw.dll");
        }
        else if (((messageId & (7 << 16)) && (messageId & 0xFFFF) == 0x103) || ((messageId & (4 << 16)) || (messageId & 0xFFFF) == 0x3FF))
        {
            sourceMdl = LoadLibraryW(L"dinputd.dll");
        }
        //else if ()
        //{
        //
        //}
        if ((messageId & (0x876 << 16))) // D3D types errores
        {
            //Directdraw For codes definition look in ddraw.h
            if (((messageId & 0xFFFF) > 0x4 && (messageId & 0xFFFF) < 0x2BC))
            {
                sourceMdl = LoadLibraryW(L"ddraw.dll");
            }
            //D3D9 For codes definition look in d3d9.h
            else if (((messageId & 0xFFFF) > 0x817 && (messageId & 0xFFFF) <= 0x884) && ((messageId & 0xFFFF) == 0x17c || (messageId & 0xFFFF) == 0x21C))
            {
                sourceMdl = LoadLibraryW(L"D3D9.dll");
            }
        }
        else if ((messageId & (0x879 << 16)) || (messageId & (0x880 << 16))) // D3D10
        {
            sourceMdl = LoadLibraryW(L"D3D10.dll");
        }

    }

    const DWORD ret_size = FormatMessageW(flag, sourceMdl, messageId, langId, (wchar_t *) &inner_allocated_buf, 0, nullptr);
    return {};
}

LIB_EXPORT
std::wstring ForamtSystemMessage(const uint32_t messageId) noexcept
{
    const DWORD curr_last_err = GetLastError();
    DWORD lang_id = 0;

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
    ForamtMessage(messageId, lang_id);
    SetLastError(curr_last_err);
    //std::wstring(inner_allocated_buf, static_cast<const size_t>(ret_size) - 2);
    return {};
}

#endif