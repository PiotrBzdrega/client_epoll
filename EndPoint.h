#pragma once
// #include <functional>
#include <string_view>
#include <vector>
#include <thread>

#include "IPeer.h"
#include "IManager.h"

#include "ThreadSafeQueue.h"

#include <sys/epoll.h> //epoll

constexpr auto MAXEVENTS = 64;

namespace COM
{
    class EndPoint : public IManager
    {
        //TODO: Thread Pool needed to handle responses from Epool waiter
        //TODO: Cannot read and write simultaneously for same fd, insert some mutex for each EndPoint
    private:
        class Epool;
        // std::string_view _ip;
        // uint16_t _port;
        std::vector<std::unique_ptr<IPeer>> _peer; //TODO: use unordered map if needed
        std::thread stdIN; /* execute msg's from queue */
        void interpretRequest(std::shared_ptr<std::string> arg);
    public:
        EndPoint (std::string_view &ip, std::string_view &port, ThreadSafeQueue<std::string> &queue);
        ~EndPoint ();
        bool appendPeer(int fd, uint32_t param) override;
        void addPeer(std::string_view ip, std::string_view port, bool ssl);
        template<typename... Args>
        IO& find(Args... args);
        // void accept (sockaddr *addr, socklen_t *addr_len);
        EmptyDB a; //should not build, try to create only one instance of it
    };

    class EndPoint::Epool
    {
        
    private:
        int _fd = CLOSED; /* epool file descriptor */
        std::thread waiter; /* provides events to file descriptors */
        int waiterFunction();
    public:
        Epool();
        ~Epool();
        int operator()() {return _fd;}
        bool addNew(int fd, uint32_t param);
    };

}


