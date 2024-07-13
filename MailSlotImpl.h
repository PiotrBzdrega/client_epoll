#pragma once

#include <winsock2.h>
#include <windows.h>
#include <cstdio>

inline HANDLE initReceiver(const wchar_t *ms_name)
{
    // Create a mailslot
    HANDLE hMailslot = CreateMailslot(ms_name,
                                0, // no maximum message size
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
    // Open a handle to the mailslot
    HANDLE hMailslot = CreateFile(
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

inline readMailPost(HANDLE handle,)
{

}