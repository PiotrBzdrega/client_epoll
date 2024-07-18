#pragma once


#ifdef  __linux__
#include <sys/epoll.h>
using SOCKET=int;
#elif _WIN32
#include <winsock2.h> //closesocket
#endif

class IManager
{
private:
    /* data */
public:
    /* edge-triggered read able to detect peer shutdown */
    virtual bool appendNotificationNode(SOCKET fd, uint32_t param = EPOLLIN | EPOLLET  | EPOLLRDHUP) = 0;
    virtual ~IManager() = default;
};