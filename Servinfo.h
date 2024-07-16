#pragma once


#ifdef  __linux__
#include <netdb.h> //freeaddrinfo
#elif _WIN32
#include <ws2tcpip.h> //freeaddrinfo
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