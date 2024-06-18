#include "IO.h"
#include "Servinfo.h"

#include <sys/socket.h> //connect, bind
#include <arpa/inet.h> // inet_ntop/_pton
#include <fcntl.h>

#include <iostream>
#include <cstring>


static int closeFd(int &fileDescriptor)
{   
    std::fprintf(stderr,"Close fd: %u\n",fileDescriptor);
    return ::close(fileDescriptor);
};

namespace COM
{
    IO::IO(std::string_view ip, uint16_t port, bool ssl, IManager* manager, ILog* logger, IDB* db) : IPeer(manager,logger,db), _locIp{ip}, _locPort{port}
    {
        if (ssl)
        {
            /* SSL factory context to create SSL objects*/
            _tls.create_context();

            _tls.configure_context();

            /* create object representing TLS connection */
            _tls.create_object();
        }
    }
    bool IO::request(std::string_view ip, uint16_t port, int fd, req request, std::string msg /*query &item*/)
    {
        //TODO: make in the future only one instance that handle read/write/close/connect/ with list of available masters state/ip/port/fd
        //try to create it
        // In Linux, if you call connect on a file descriptor that is already connected to a socket, the behavior is specified by the POSIX standard and generally results in an error. Here's what typically happens:

        // Error Returned: The connect function will fail, and it will set the errno to EISCONN to indicate that the socket is already connected. The return value of the connect call will be -1.

        // No Side Effects: The state of the socket remains unchanged. The existing connection remains valid and continues to function as expected.

        if ( ((!ip.empty() && ip == _ip && port !=0 && port==_port) || (fd != CLOSED && fd == _fd)) /* match with file descriptor or ip address*/
        ||
            (!ip.empty() && port !=0 && _fd == CLOSED ) /* not initialized connection*/
        )
        {
            /* forward request and potential message*/
            _que.push(std::make_tuple(request,msg ));
        }
        
        return false;
    }

    int IO::connectTCP()
    {
        int ret;
 
        struct addrinfo hints = {0}; 
        {
            hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
            hints.ai_socktype = SOCK_STREAM; /* TCP */
        }

        Servinfo servinfo;
        /* Translate name of a service location and/or a service name to set of socket addresses*/
        ret = getaddrinfo(      _locIp.data(), //"172.22.64.1",  //"localhost", /* e.g. "www.example.com" or IP */
                                std::to_string(_locPort).data(), /* e.g. "http" or port number  */
                                &hints, /* prepared socket address structure*/
                                &servinfo); /* pointer to sockaddr structure suitable for connecting, sending, binding to an IP/port pair*/

        if (ret) 
        {
            std::cout<<"EAI error: "<<gai_strerror(ret)<<"\n";
            handle_error("getaddrinfo");
        }

        int fd = CLOSED; /* client file descriptor */

        for(struct addrinfo *ptr = servinfo; ptr != nullptr;  ptr= ptr->ai_next)
        { 
            char host[256],service[256];
            ret = getnameinfo(ptr->ai_addr, ptr->ai_addrlen,
                        host,sizeof(host),
                        service,sizeof(service),0);

            if (ret) 
            {
                std::cout<<"EAI error: "<<gai_strerror(ret)<<"\n";
                // handle_error("getnameinfo");
                // continue;
                //TODO: Temporary failure in name resolution happend from time to time  
            }

            /* IPv4 and IPv6 addresses from binary to text form*/
            char addr[INET6_ADDRSTRLEN]={0};
            switch (ptr->ai_family)
            {
            case AF_INET:
                inet_ntop(ptr->ai_family,&(reinterpret_cast<struct sockaddr_in *>(ptr->ai_addr)->sin_addr),addr,sizeof(addr));
                break;

            case AF_INET6:
                inet_ntop(ptr->ai_family,&reinterpret_cast<struct sockaddr_in6 *>(ptr->ai_addr)->sin6_addr,addr,sizeof(addr));
                break;
            }

            if (addr == nullptr) { handle_error("getnameinfo"); continue; }


            std::cout<< "\nIP: "<< addr <<
                "\nhost: "<<  host <<
                "\nport/service: "<< service <<
                "\ntransport layer: " << ((ptr->ai_socktype == SOCK_STREAM) ? "TCP" : (ptr->ai_socktype == SOCK_DGRAM) ? "UDP" : "OTHER")<<
                "\nprotocol: "<<ptr->ai_protocol<<
                "\n" ;


            fd = socket(ptr->ai_family,ptr->ai_socktype,ptr->ai_protocol);
            if (fd == CLOSED) 
            { 
                handle_error("socket"); 
                continue;
            }

/* modify the behavior of the socket */
            int on = 1;
#ifdef __linux__
            ret = setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(char *)&on,sizeof(on)); /* SO_REUSEADDR allows your server to bind to an address which is in a TIME_WAIT state */
#elif _WIN32
            ret = setsockopt(fd,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,(char *)&on,sizeof(on)); /* The SO_EXCLUSIVEADDRUSE option prevents other sockets from being forcibly bound to the same address and port */
#endif
            if (ret == -1)
            {
              closeFd(fd);
              handle_error("setsockopt");
              continue;
            }

            //TODO: bind if local ip/port will be available

            if (!_locIp.empty() || _locPort != 0)
            {
                struct sockaddr_in sa;

                // Zero out the sockaddr_in structure
                std::memset(&sa, 0, sizeof(sa));

                sa.sin_family = AF_INET;  // IPv4
                sa.sin_port = htons(_locPort);

                if (inet_pton(AF_INET, _locIp.data(), &(sa.sin_addr)) != 1) 
                {
                    closeFd(fd);
                    handle_error("inet_pton");          
                    continue;
                }
                ret = bind(fd, (struct sockaddr*)&sa, sizeof(sa));
                if (ret != 0)
                {
                    closeFd(fd);
                    handle_error("bind");          
                    continue;
                }
            }


            ret = setNonBlock(fd);
            if(ret == -1)
            {
                /* setting File Descriptor as nonblock failed */
                continue;
            }

            if (::connect(fd, ptr->ai_addr, ptr->ai_addrlen) == -1) 
            {
                if (errno != EINPROGRESS)
                {
                    closeFd(fd);
                    handle_error("connect");
                    continue;
                }
                else
                {
                    /* Connection is in progress, wait for it to complete */ 
                    struct timeval tv;
                    tv.tv_sec = 10; // Timeout value (10 seconds)
                    tv.tv_usec = 0;

                    fd_set writeset, exceptset;
                    // Initialize the set
                    FD_ZERO(&writeset);
	                FD_SET(fd, &writeset);

                    ret = select(fd+1, nullptr, &writeset, &exceptset, &tv); //
                    if (ret == -1)
                    {
                        closeFd(fd);
                        handle_error("select");
                        continue; 
                    }
                    else
                    if (ret == 0)
                    {
                        /* time-out */
                        closeFd(fd);
                        handle_error("select time-out");
                        continue;
                    }
                    else
                    {
                        /* Check if the socket is writable */
                        if(FD_ISSET(fd, &writeset))
                        {
                            /* Socket is writable check errors */
                            int optval;
                            socklen_t optlen = sizeof(optval);
                            ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, &optlen);
                            if (ret == -1)
                            {
                                closeFd(fd);
                                fd = CLOSED;
                                handle_error("getsockopt");
                                continue;
                            }
                            else
                            if (optval !=0)
                            {
                                /* Error in connection */
                                closeFd(fd);
                                fd = CLOSED;
                                handle_error("optval error");
                                continue;
                            }
                            else
                            {   
                                /* succesfully connected */
                            }
                        }
                        else
                        {
                            /* Socket not writable */
                            closeFd(fd);
                            fd = CLOSED;
                            handle_error("FD_ISSET");
                            continue;
                        }
                    }
                }
            }
            /* leave loop with first succesfully created socket*/
            break;
        }

        if (fd == CLOSED)
        {
            std::cout << "Server with specified settings not available\n";
        }

        return fd;
    }

    int IO::setNonBlock(int fd)
    {
        int ret{};
        /* get file descriptor flags */
        ret = fcntl(fd,F_GETFL,0);
        if(ret == -1)
        {
            //TODO: handle removal from peers if already appended
            handle_error("fctl-get");          
        }
        /* set as non-blocking if it is not set yet */
        auto flags = ret | O_NONBLOCK;
        ret = fcntl (fd, F_SETFL, flags);
        if(ret == -1)
        {
            //TODO: handle removal from peers if already appended
            handle_error("fctl-set");
        }
        return ret;
    }
    void IO::threadFunc()
    {
        while (1)
        {
            /* if queue is empty -> wait */
            auto query =_que.pop();
            
 
            auto request = std::get<req>((*query));
            auto message = std::get<std::string>(*(query));

            /* drop close, read, write if disconnected*/
            if ( _fd == CLOSED && request != req::CONNECT) { continue; }

            if (request == req::CONNECT)
            {
                while (_fd==CLOSED)
                {
                    connect();
                    sleep(5);
                }
            }
            else
            if (request == req::READ)
            {
                int all_reads{};
                while (1)
                {
                    auto res = read();
                    if (res == -1)
                    {
                        /* errno == EAGAIN, that means we have read all data */
                        break;
                    }
                    else
                    if(res == 0)
                    {   
                        /* erase all elements */
                        _que.clear();

                        /* allow to get to close statement */
                        request = req::CLOSE;
                    }
                    else
                    {
                        /* Successful read*/
                        all_reads+= res;
                        _logger->log(""); //TODO: create logger
                        _db->set(0,0); //TODO: update DB
                        // logger/callback_to_update_db
                    }                   
                }
            }
            else
            if (request == req::WRITE)
            {
                write(message.data(), message.size());
            }
            if (request == req::CLOSE)
            {
                close();
            }               
        }
    }
    bool IO::connect()
    {   
        _fd = connectTCP();
        if (_fd != CLOSED)
        {
            if (_tls()==nullptr || _tls.connect(_fd) )
            {      
                notifyManager();
                return true;
            }        
        }
        
        return false;
    }

    bool IO::close()
    {   
        closeFd(_fd);
        _fd = CLOSED;

        // static_assert(false,"close tls");
        return false;
    }

    int IO::read()
    {
        SSL *ssl_obj = _tls();

        int res;

        if(ssl_obj != nullptr)
        {
            /* TLS */
            res=SSL_read(ssl_obj, _buffer, MAX_READ);
            if (res <= 0)
            {
                switch(_tls.handle_io_failure(res))
                {
                case -1 :
                    /* some error appears, but anyway socket must be closed, then modify res */
                    std::printf("Failed to read\n");
                    res = 0;
                    break;
                    
                case 0 :
                    std::printf("End of file\n");
                    break;
                }
            }
        }
        else
        {
            /* unsecure TCP */
            res = ::read(_fd,_buffer, MAX_READ);
            if (res== -1 && errno != EAGAIN)
            {
                /* some error appears, but anyway socket must be closed, then modify res */
                handle_error("read");
                res=0;
            }
            else
            if (res ==0)
            {
                std::printf("End of file\n");
            }  
        }
        return res;
    }

    int IO::write(const void* buffer, int count)
    {
        SSL *ssl_obj = _tls();

        int res{};

        //TODO: handle partial write; what if write returns 0
        // const char* buf = static_cast<const char*>(buffer);
        // while (res<count)
        // {
        //     if(ssl_obj != nullptr)
        //     {   
        //         /* TLS */
        //         res += SSL_write(ssl_obj, buf+res, count-res);
        //     }
        //     else
        //     {
        //         /* unsecure TCP */
        //         res += ::write(_fd,buf+res,count-res);
        //     }       
        // }       
        // return res;

        if(ssl_obj != nullptr)
        {   
            /* TLS */
            res = SSL_write(ssl_obj, buffer, count);
        }
        else
        {
            /* unsecure TCP */
            res = ::write(_fd,buffer,count);
        }

        return res;
    }
}