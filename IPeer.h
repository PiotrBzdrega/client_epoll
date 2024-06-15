#pragma once

#include <string>
#include "IManager.h"
#include "ILog.h"
#include "IDB.h"

#include "EmptyImplementation.h"

constexpr auto CLOSED = -1;
enum class req {CONNECT, CLOSE, WRITE, READ};

class IPeer
{
private:
    IManager* _manager;
protected:
    std::string _ip;
    uint16_t _port;
    int _fd = CLOSED; /* endpoint file descriptor */
    ILog* _logger; 
    IDB* _db;
    void notifyManager()
    {
        _manager->appendPeer(_fd);
    };
public:
    IPeer(IManager* manager, ILog* logger = &emptyLogger, IDB* db = &emptyDB) : _manager(manager), _logger(logger), _db(db){};
    virtual bool request(std::string ip, uint16_t port, int fd, req request,std::string_view msg) = 0;
    virtual ~IPeer() = default;
};

