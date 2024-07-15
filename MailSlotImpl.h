#pragma once

#ifdef __linux__
#elif _WIN32
#include <winsock2.h>
#include <windows.h>
#endif
#include <cstdio>
#include <cstdlib>

inline HANDLE initReceiver(const wchar_t *ms_name)
{
    // Create a mailslot
    HANDLE hMailslot = CreateMailslotW(ms_name,
                                0, // no limitation of maximum message size
                                MAILSLOT_WAIT_FOREVER, // no time-out
                                NULL); // default security attributes

    if (INVALID_HANDLE_VALUE != hMailslot) 
    {
        std::printf("\nCreateMailslot() was successful.");
        return hMailslot;
    }
    
    return nullptr;  //Error
}

inline HANDLE initSender(const wchar_t *ms_name)
{

    /* wchar_t* to char* */
    size_t len = std::wcstombs(nullptr, ms_name, 0);



    // Open a handle to the mailslot
    HANDLE hMailslot = CreateFileW(
        ms_name, // Mailslot name
        GENERIC_WRITE,                       // Write access
        FILE_SHARE_READ,                     // Shared read access
        nullptr,                             // Default security attributes
        OPEN_EXISTING,                       // Opens an existing mailslot
        FILE_ATTRIBUTE_NORMAL,               // Normal file attributes
        nullptr                              // No template file
    );

    if (INVALID_HANDLE_VALUE != hMailslot) 
    {
        std::printf("\nCreateFile() was successful.");
        return hMailslot;
    }

    return nullptr;  //Error
}
