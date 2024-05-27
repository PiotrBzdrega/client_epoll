#pragma once
 
#include "TLS.h"
#include <unistd.h> //close, read, write
#include <string_view>
#include <thread>

constexpr auto CLOSED = -1;

static void handle_error(const char* msg, bool exit_proc=false)
{
    perror(msg); 
    if(exit_proc) {exit(EXIT_FAILURE);}
};

static int closeFd(int fileDescriptor)
{   
    std::fprintf(stderr,"Close fd: %u\n",fileDescriptor);
    return ::close(fileDescriptor);
};

namespace COM
{
    class IO
    {   
    private:
        TLS _tls;
        std::string_view _ip;
        std::string_view _port;
        int _fd = CLOSED; /* endpoint file descriptor */
        int connectTCP();
        int setNonBlock(int fd);
        std::thread servant;
    public:
        IO(std::string_view &ip, std::string_view &port, bool ssl);
        int operator()() {return _fd;}
        const std::string_view getIp() {return _ip;}
        bool connect ();
        bool close ();
        int read(void *buffer, int count);
        int write(const void* buffer, int count);
    };
}

 