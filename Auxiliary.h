#pragma once

#include <stdio.h>
#include <cstdlib>
#ifdef  __linux__
#include <unistd.h>
#elif _WIN32 //available for both x64 and x32
#endif

inline void handle_error(const char* msg, bool exit_proc=false)
{
    perror(msg); 
    if(exit_proc) {exit(EXIT_FAILURE);}
};

inline int closeFd(int &fileDescriptor)
{   
    std::fprintf(stderr,"Close fd: %u\n",fileDescriptor);
    return ::close(fileDescriptor);
};