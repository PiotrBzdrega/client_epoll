#pragma once
 
#include "TLS.h"
#include "IPeer.h"
#include "ThreadSafeQueue.h"

#include <unistd.h> //close, read, write
#include <string_view>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <tuple>

constexpr auto CLOSED = -1;
constexpr auto MAX_READ = 2048;

using query = std::tuple<req,std::string>;

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
    class IO : public IPeer
    {   
    private:
        TLS _tls;
        std::string_view _ip;
        std::string_view _port;
        int _fd = CLOSED; /* endpoint file descriptor */
        char _buffer[MAX_READ]{};
        int connectTCP();
        int setNonBlock(int fd);
        std::thread servant;
        void threadFunc();
        ThreadSafeQueue<query> _que;
    public:
        IO(std::string_view &ip, std::string_view &port, bool ssl);
        int operator()() {return _fd;}
        const std::string_view getIp() {return _ip;}
        bool connect ();
        bool close ();
        int read();
        int write(const void* buffer, int count);
    };
}

 