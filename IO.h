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

namespace COM
{
    constexpr auto MAX_READ = 2048;

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
        ~IO();
        // int operator()() {return _fd;}
        bool request(std::string_view ip, uint16_t port, int fd,  req request, std::string msg ) override;
        void timerCallback() override;
        const std::string_view getIp() {return _locIp;}
        bool connect ();
        bool close ();
        int read();
        int write(const void* buffer, int count);
    };
}

 