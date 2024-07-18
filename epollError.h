#pragma once

#include <sys/epoll.h>
#include <string>

#include <array>


constexpr std::array events =
{
    std::pair<EPOLL_EVENTS, std::string_view>{EPOLLIN,"EPOLLIN"}
};

constexpr std::string_view epoll_strerror(uint32_t events)
{   
    constexpr auto a{"ada"};
    std::string_view adam();

    if (events & EPOLLIN)
    {
        
    }
    
}
