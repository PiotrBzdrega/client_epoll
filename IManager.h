#pragma once


#ifdef  __linux__
#include <sys/epoll.h>
#endif

class IManager
{
private:
    /* data */
public:
    /* edge-triggered read able to detect peer shutdown */
    virtual bool appendNotificationNode(int fd, uint32_t param = EPOLLIN | EPOLLET  | EPOLLRDHUP) = 0;
    virtual ~IManager() = default;
};