#pragma once

#include <windows.h>
#include <string_view>

namespace COM
{
    constexpr auto defaultLockName=L"lock.pid";

    class Lock
    {
    private:
        HANDLE _handle;
        int createFile(const std::wstring_view fileName);
        int lock();
        void unlock();
    public:
        Lock(const std::wstring_view fileName = defaultLockName);
        ~Lock();
    };
}


