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

constexpr auto MAX_READ = 2048;

static void handle_error(const char* msg, bool exit_proc=false)
{
    perror(msg); 
    if(exit_proc) {exit(EXIT_FAILURE);}
};

namespace COM
{
    class IO : public IPeer
    {   
    private:
        TLS _tls;
        std::string _locIp;
        uint16_t _locPort;
        char _buffer[MAX_READ]{}; //TODO:change for std::array
        int connectTCP();
        int setNonBlock(int fd);
        std::thread servant;
        void threadFunc();
        ThreadSafeQueue<query> _que;
    public:
        IO(std::string_view ip, uint16_t port, bool ssl, IManager* manager, ILog* logger, IDB* db);
        // int operator()() {return _fd;}
        bool request(std::string ip, uint16_t port, int fd, query &item) override;
        const std::string_view getIp() {return _locIp;}
        bool connect ();
        bool close ();
        int read();
        int write(const void* buffer, int count);
    };
}

 