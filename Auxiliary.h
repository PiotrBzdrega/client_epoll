#pragma once

#include <stdio.h>
#include <cstdlib>
#ifdef  __linux__
#include <unistd.h>
using SOCKET=int;
#elif _WIN32
#include <winsock2.h> //closesocket
#endif

inline void handle_error(const char* msg, bool exit_proc=false)
{
    perror(msg); 
    if(exit_proc) {exit(EXIT_FAILURE);}
};

inline int closeFd(SOCKET &fileDescriptor)
{   
    std::fprintf(stderr,"Close fd: %u\n",fileDescriptor);
#ifdef  __linux__
    return ::close(fileDescriptor);
#elif _WIN32
    return closesocket(fileDescriptor);
#endif
};