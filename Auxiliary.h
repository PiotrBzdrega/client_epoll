#pragma once

#include <stdio.h>
#include <cstdlib>
#include <unistd.h>

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