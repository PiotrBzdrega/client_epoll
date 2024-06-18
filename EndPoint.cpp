#include "EndPoint.h"
#include "IPeer.h"
#include "IO.h"

#include <stdexcept> // for std::out_of_range
#include <sstream> //istringstream
#include <iostream> //std::cerr
#include <string_view> //string_view
#include <charconv> //std::from_chars
#include <regex>
#include <tuple>



struct addrTCP
{
    std::string_view ip;
    uint16_t port;

    bool
    isEmpty() const{ return port && !ip.empty() ;}
};

static addrTCP validateAddrTCP(std::string_view addr, ILog* logger = &emptyLogger)
{
    uint16_t port{};

    auto portSep = addr.find(":");
    if(portSep == addr.npos)
    { 
        logger->log("Did not find : delimiter\n");
        return {"",0}; 
    }

    if(std::from_chars(addr.data()+portSep+1,addr.data()+addr.size(),port).ec != std::errc() || !port )
    {
        /* wrong */
        logger->log("not valid port in address %s\n",std::string(addr.data()+portSep+1,addr.data()+addr.size()).data());
        return {"",0}; 
    }


    std::regex pattern("\\.");
    std::vector<std::string> tokens(
    std::sregex_token_iterator(addr.begin(), addr.begin() + portSep, pattern, -1),
    std::sregex_token_iterator()
    );

    std::array<uint8_t,4> octet;

    int idx{};
    for( auto token : tokens)
    {
        if(std::from_chars(token.data(),token.data()+token.size(),octet[idx]).ec != std::errc() || (!idx && !octet[idx]) ) // first octet cannot have 0
        {
            logger->log("Invalid IP address element: %s\n",token.data());
            return {"",0};
        }
        // else
        // {
            
        //     std::printf("%d ",octet[idx]);
        // }
        idx++;
    }

    return {std::string(addr.begin(),addr.begin()+portSep),port};
}

namespace COM
{
//TODO: ideas: 1. events from epool (read,close) should be push to the same queue as stdin, but with a higher priority (push to the front if queue not empty)
//TODO:        2. should each IO has own thread that will be notify(condition variable) to execute close/read/write (connect in loop till connected rejecting all other requests) ?
//TODO:        3. what if connection will be closed? thread should be closed or ready to aquire by other peer? probably stay idle if Thread Pool will be provided by EndPoint, otherwise IO will contain own thread
//TODO:        4. what about synchronous data transfer, how to insert single request in order to not mix up sequence read <-> write
//TODO:        5. try maybe icmp before blind reconnect for training
//TODO:        6. IO should contain waiting condition variable, EndPoint should keep lock as long as no request are comming

    EndPoint::EndPoint(std::string_view ip, uint16_t port, ThreadSafeQueue<std::string> &queue,ILog* logger, IDB* db) : _logger(logger), _db(db)
    {
        /* create new Peer */
        _peer.emplace_back(std::make_unique<IO>(ip,port,*this));

        // stdIN = std::thread(&EndPoint::stdINLoop,this,queue);
        /* wait for new message */
        stdIN = std::thread([&]{
            while (1)
            {
                interpretRequest(queue.pop());
            }    
        });
    } 

    EndPoint::~EndPoint()
    {
        if(stdIN.joinable())
        {   
            /* wait for stdIN to */
            stdIN.join();
        }
    }

    bool EndPoint::appendNotificationNode(int fd, uint32_t param)
    {
        return false;
    }

    void EndPoint::interpretRequest(std::shared_ptr<std::string> arg)
    {


        //-i 172.22.77.70:112 -m adam (write to ip)
        //-f 3 -r (read from file descriptor)
        //-f 4 -c (close file descriptor)
        //-i 172.22.77.70:2111 (connect with ip)
        req notification;
        std::istringstream iss(*arg);
        std::string_view opt;
	    std::string word;
        std::string_view ip; /*-i*/
        int fd; /*-f*/
        uint16_t port;
        std::string msg; /*-m*/
	    while (iss >> word )
        {
            if (word=="-i" )
            {
                /* IP */
                if (iss >> word )
                {
                    auto [ip,port] =validateAddrTCP(word,_logger);
                }
                else
                {
                    throw std::runtime_error("missing ip in arguments");
                }         
                notification = req::CONNECT;
            }
            else
            if (word=="-f")
            {
                /* FILE DESCRIPTOR */
                if(iss >> word )
                {
                    auto data = word.data();
                    if (std::from_chars(data,data+word.size(),fd).ec != std::errc() || !fd)
                    {
                        std::printf("Not valid file descriptor: %s\n",data);
                    }
                }
                else
                {
                    throw std::runtime_error("missing file descriptor number");
                }
            }
            else
            if (word=="-m" || opt == "-m")
            {
                /* MESSAGE */
                if (opt != "-m")
                {
                    opt="-m";
                    msg=word;
                    notification = req::WRITE;
                }
                
                msg+=" "+word;               
            }
            else
            if (word=="-r")
            {
                /* READ */
                notification = req::READ;
            }
            if (word=="-c")
            {
                /* CLOSE */
                notification = req::CLOSE;
            }
            else
            {
                _logger->log("Not expected argument : %s\n",word.data());
                return;
            }
        }
        _logger->log("IP: %s MSG: %s\n",ip.data(),msg.data());


            bool res{};
            for ( int i=0; i< _peer.size() && res==false ; i++)
            {
                res=_peer[i].get()->request(ip,port,fd, notification,msg);
            }
            if (!res)
            {
                /* peer did not found*/
                _peer.emplace_back(std::make_unique<IO>(ip,port,*this));
            }
        
    }

    // template<typename... Args>
    // IO &EndPoint::find(Args... args)
    // {
    //     for (auto &io : _peer)
    //     {

    //         ([&] { 
    //                 if constexpr (std::is_same_v<decltype(args), int>)
    //                 {
    //                     if (io() != 0 && io()==args) {return io;}
    //                 }
    //                 else
    //                 if constexpr (std::is_same_v<decltype(args), std::string_view>)
    //                 {
    //                     if (!io.getIp().empty() && io.getIp()==args) {return io;}
    //                 }
    //              }(), ...);
    //     }
        
    //     throw std::out_of_range("File Descriptor not found");
    // }

    // void EndPoint::accept(sockaddr *addr, socklen_t *addr_len)
    // {
    //     while (1)
    //     {
    //         struct sockaddr in_addr;
    //         socklen_t in_len;
    //         in_len = sizeof in_addr;
    //         int fd_in = ::accept(_fd, &in_addr, &in_len);
    //         if (fd_in == -1)
    //         {
    //             printf("errno=%d, EAGAIN=%d, EWOULDBLOCK=%d\n", errno, EAGAIN, EWOULDBLOCK);
    //             if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
    //             {
    //                 std::printf ("processed all incoming connections.\n");
    //                 break;
    //             }
    //             else
    //             {
    //                 perror ("accept");
    //                 break;
    //             }

    //         }

    //         /* IPv4 and IPv6 addresses from binary to text form*/
    //         char addr[INET6_ADDRSTRLEN]={0};
    //         switch (in_addr.sa_family)
    //         {
    //         case AF_INET:
    //             inet_ntop(in_addr.sa_family,&(reinterpret_cast<struct sockaddr_in *>(in_addr.sa_data)->sin_addr),addr,sizeof(addr));
    //             break;
    //         case AF_INET6:
    //             inet_ntop(in_addr.sa_family,&reinterpret_cast<struct sockaddr_in6 *>(in_addr.sa_data)->sin6_addr,addr,sizeof(addr));
    //             break;
    //         }
    //         std::printf("Incomming connection from %s\n",addr);

    //         setNonBlock(fd_in);
    //     }
    // }

    EndPoint::Epool::Epool(ThreadSafeQueue<std::string> &queue)
    {
        _fd = epoll_create1(0);
        if (_fd == -1)
        {
            handle_error("epoll_create1");
            return;    
        }

        /* start off waiter thread*/
        waiter = std::thread(&Epool::waiterFunction,this,queue);
    }
    EndPoint::Epool::~Epool()
    {
        if (waiter.joinable())
        {
            waiter.join();
        }
    }
        bool EndPoint::Epool::addNew(int fd, uint32_t param)
    {
        /* setup event for client socket*/
        struct epoll_event event;

        event.data.fd = fd;
        event.events = param;
        /*  Add a file descriptor to the interest list of the epoll file descriptor, fd_epoll */
        if (epoll_ctl(_fd, EPOLL_CTL_ADD ,fd, &event) == -1)
        {
            handle_error("epoll_ctl_client");  
            return false;    
        }
        return true;
    }

    int EndPoint::Epool::waiterFunction(ThreadSafeQueue<std::string> &queue)
    {
        struct epoll_event *events;
        /* clean container that stores events returned  by wait*/
        events = (struct epoll_event*)  calloc (MAXEVENTS, sizeof(struct epoll_event));

        // char buf[MAX_READ]={0};

        while (1)
        {
            auto wait = epoll_wait(_fd, events, MAXEVENTS, -1); //wait infinite time for events
            if (wait == -1)
            {
                handle_error("epoll_wait");      
            }
            for (size_t i = 0; i < wait; i++)
            {
                std::string fd ="-f "+ std::to_string(events[i].data.fd)+ " "; 
                /* error or hang up happend */
                if ((events[i].events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) && 
                (!(events[i].events & EPOLLIN))) /* TODO: some servers sends msg then close connection an 8193 is obtained should i close and  */
                {
                    fprintf (stderr, "epoll error. events=%u\n", events[i].events); //TODO: event string

                    /* pass close request with file descriptor */
                    queue.push(fd+"-c");
	                continue;
                }
                else
                {
                    /* pass read request with file descriptor */
                    queue.push(fd+"-r");
                }
            }
        }
        return 0;
    }
}
