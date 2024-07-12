#pragma once

#ifdef  __linux__
#include <sys/epoll.h> //epoll
#endif

#include <thread>
#include "ThreadSafeQueue.h"


namespace COM
{
    constexpr auto CLOSED = -1;
    constexpr auto MAXEVENTS = 64;
    class Epoll
    {

        private:
            int _fd = CLOSED; /* epool file descriptor */
            std::thread waiter; /* provides events to file descriptors */
            int waiterFunction(ThreadSafeQueue<std::string> &queue);
        public:
            Epoll(ThreadSafeQueue<std::string> &queue);
            ~Epoll();
            int operator()() {return _fd;}
            bool addNew(int fd, uint32_t param);
    };

}
