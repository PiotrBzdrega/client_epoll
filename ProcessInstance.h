#pragma once

#ifdef  __linux__
    using HANDLE = int
#elif _WIN32 //available for both x64 and x32
#include "windows.h"
#endif


namespace COM
{
    constexpr auto mutexName=L"FirstInstance";
    class ProcessInstance
    {
    private:
        HANDLE _handle;
        bool isParent;
    public:
        ProcessInstance() : isParent(false)
        {
            _handle=CreateMutexW(NULL,FALSE,mutexName);
            isParent=_handle != nullptr;
        }
        ~ProcessInstance()
        {
            if (_handle)
            {
                CloseHandle(_handle);
            }
        }
        bool operator()()
        {
            return isParent;
        }
    };

}

