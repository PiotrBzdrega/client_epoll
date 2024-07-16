#pragma once

#include <string>
#include "IManager.h"
#include "ILog.h"
#include "IDB.h"

#ifdef  __linux__
using SOCKET=int;
#elif _WIN32
#include <winsock2.h> //closesocket
#endif



constexpr auto CLOSED = -1;
enum class req {CONNECT, CLOSE, WRITE, READ};

using query = std::tuple<req,std::string>;

class IPeer
{
private:
    IManager* _manager;
protected:
    std::string _ip;
    uint16_t _port;
    SOCKET _fd{CLOSED}; /* endpoint file descriptor */
    ILog* _logger; 
    IDB* _db;
    void notifyManager()
    {
        _manager->appendNotificationNode(_fd);
    };
public:
    IPeer(IManager* manager, ILog* logger, IDB* db) : _manager(manager), _logger(logger), _db(db){};
    virtual bool request(std::string_view ip, uint16_t port, SOCKET fd,  req request, std::string msg ) = 0;
    virtual void timerCallback() = 0;
    virtual ~IPeer() = default;
};

