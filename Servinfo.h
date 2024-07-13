#pragma once


#ifdef  __linux__
#include <netdb.h> //freeaddrinfo
#elif _WIN32 //available for both x64 and x32
#endif

class Servinfo
{
private:
    struct addrinfo *data=nullptr;
public:
    Servinfo(){};
    ~Servinfo(){ freeaddrinfo(data); };

    struct addrinfo** operator &() { return &data; }
    operator struct addrinfo*() { return data; }
};