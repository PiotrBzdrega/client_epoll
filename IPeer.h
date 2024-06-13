#pragma once

#include <string_view>

enum class req {CONNECT, CLOSE, WRITE, READ};

class IPeer
{
private:
    std::string_view _ip;
    std::string_view _port;
public:
    virtual void request(req request,std::string_view msg) = 0;
    virtual ~IPeer() {};
};

