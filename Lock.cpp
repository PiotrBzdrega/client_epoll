#include "Lock.h"

namespace COM
{
    Lock::Lock(const std::wstring_view fileName)
    {
        createFile(fileName);
    }
    Lock::~Lock()
    {
        if (!UnlockFile(_handle, offsetLow, offsetHigh, lockLengthLow, lockLengthHigh)) 
        std::cerr << "Failed to unlock file: " << GetLastError() << std::endl;

        CloseHandle(_handle);
    }

    int Lock::createFile(const std::wstring_view fileName)
    {
        //Open a file for reading and writing
        _handle= CreateFile(fileName.data(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
            return _handle == INVALID_HANDLE_VALUE ? GetLastError() : EXIT_SUCCESS;
    }

    int Lock::lock()
    {
        DWORD offsetLow = 0;
        DWORD offsetHigh = 0;
        DWORD lockLengthLow = 12;
        DWORD lockLengthHigh = 0;

        if (!LockFileEx(_handle, LOCKFILE_EXCLUSIVE_LOCK |  LOCKFILE_FAIL_IMMEDIATELY
,0, offsetLow, offsetHigh, lockLengthLow, lockLengthHigh)) 
        {
            return GetLastError();
        }
        return EXIT_SUCCESS;
    }

    void Lock::unlock()
    {
    }

}