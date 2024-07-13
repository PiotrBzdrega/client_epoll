#pragma once
// #include <functional>
#include <string_view>
#include <vector>
#include <thread>

#include "IPeer.h"
#include "EmptyImplementation.h"
#include "IManager.h"
#include "Epoll.h"

#include "ThreadSafeQueue.h"

namespace COM
{

    class EndPoint : public IManager
    {
        //TODO: Thread Pool needed to handle responses from Epool waiter
        //TODO: Cannot read and write simultaneously for same fd, insert some mutex for each EndPoint
    private:
        Epoll _epoll;
        std::vector<std::unique_ptr<IPeer>> _peer; //TODO: use unordered map if needed
        std::thread stdIN; /* execute msg's from queue */
        std::thread _timer; /* calls */
        void interpretRequest(std::shared_ptr<std::string> arg);
        ILog* _logger;
        IDB* _db;

    public:
        EndPoint (std::string_view ip, uint16_t port, ThreadSafeQueue<std::string> &queue, ILog* logger = &emptyLogger, IDB* db = &emptyDB);
        ~EndPoint ();
        void join();
        bool appendNotificationNode(int fd, uint32_t param) override;
        void addPeer(std::string_view ip, std::string_view port, bool ssl);
        // template<typename... Args>
        // IO& find(Args... args);
        // void accept (sockaddr *addr, socklen_t *addr_len);
        // EmptyDB a; //should not build, try to create only one instance of it
    };
}


