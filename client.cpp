#include <iostream>
#include <string>
#include <cstring> // std::memset

#include <sys/socket.h> //connect, bind
#include <unistd.h> //close
#include <sys/file.h> //flock, open
#include <getopt.h> //getopt_long
// #include <sys/socket.h>
#include <netdb.h> //freeaddrinfo
#include <arpa/inet.h> // inet_ntop
#include <sys/timerfd.h> // timerfd
#include <sys/epoll.h> //epoll
#include <signal.h> //signal


constexpr auto MAXEVENTS = 64;
constexpr int MAX_READ = 2048;
constexpr int CLOSED = -1;

auto handle_error = [](const char* msg, bool exit_proc=false)
{
    perror(msg); 
    if(exit_proc) {exit(EXIT_FAILURE);}
};

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

int connect_socket(const std::string_view &ip, const std::string_view &port)
{   
    int ret;

    struct addrinfo hints = {0}; 
    {
        hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
        hints.ai_socktype = SOCK_STREAM; /* TCP */
    }

    Servinfo servinfo;
    /* Translate name of a service location and/or a service name to set of socket addresses*/
    ret = getaddrinfo(      ip.data(), //"172.22.64.1",  //"localhost", /* e.g. "www.example.com" or IP */
                            port.data(), /* e.g. "http" or port number  */
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
          close(fd);
          fd = CLOSED;
          handle_error("setsockopt");
          continue;
        }

//TODO: bind if local ip/port will be available
        // /* bind port to socket */
        // ret = bind(fd, ptr->ai_addr, ptr->ai_addrlen);
        // if (ret != 0)
        // {
        //   close(fd);
        //   handle_error("bind");          
        //   continue;
        // }

        /* get file descriptor flags */
        ret = fcntl(fd,F_GETFL,0);
        if(ret == -1)
        {
            close(fd);
            fd = CLOSED;
            handle_error("fctl-get");          
            continue;
        }

        /* set as non-blocking if it is not set yet */
        auto flags = ret | O_NONBLOCK;
        ret = fcntl (fd, F_SETFL, flags);
        if(ret == -1)
        {
            close(fd);
            fd = CLOSED;
            handle_error("fctl-set");
            continue;
        }

        if (connect(fd, ptr->ai_addr, ptr->ai_addrlen) == -1) 
        {
            if (errno != EINPROGRESS)
            {
                close(fd);
                fd = CLOSED;
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
                    close(fd);
                    fd = CLOSED;
                    handle_error("select");
                    continue; 
                }
                else
                if (ret == 0)
                {
                    /* time-out */
                    close(fd);
                    fd = CLOSED;
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
                            close(fd);
                            fd = CLOSED;
                            handle_error("getsockopt");
                            continue;
                        }
                        else
                        if (optval !=0)
                        {
                            /* Error in connection */
                            close(fd);
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
                        close(fd);
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

int main(int argc, char *argv[])
{
    int ret;
    std::string_view ip, port;

#ifdef __linux__
    int pid_file = open("/var/run/client.pid", O_CREAT | O_RDWR, 0666); //TODO:why 666?
    ret = flock(pid_file, LOCK_EX | LOCK_NB);
    if(ret) {
        if(EWOULDBLOCK == errno)
        {
            std::cout<<"\033[0;91m another instance is running \033[0m\n";
            handle_error("flock",true);
        }
    }
#elif _WIN32

#endif

    int option_index = 0;
    static struct option long_options[] = {
            {"ip",      required_argument, 0, 'i' },
            {"port",    required_argument, 0,  'p' },
            {0,         0,                 0,  0 }
        };

    while ((ret = (getopt_long(argc,argv,"i:p:",long_options, &option_index))) != -1)
    {
        switch (ret)
        {
        case 0:
          /* If this option set a flag, do nothing else now. */
          if (long_options[option_index].flag != 0)
            break;
          printf ("option %s", long_options[option_index].name);
          if (optarg)
            printf (" with arg %s", optarg);
          printf ("\n");
          break;

        case '?':
          printf ("Unknown argument\n");
          break;

        case 'i':
          printf ("option -i with value `%s'\n", optarg);
          ip=optarg;
          break;

        case 'p':
          printf ("option -p with value `%s'\n", optarg);
          port = optarg;
          break;
        }
    }

    /* Print any remaining command line arguments (not options). */
    if (optind < argc)
    {
      printf ("non-option ARGV-elements: ");
      while (optind < argc)
        printf ("%s ", argv[optind++]);
      putchar ('\n');
    }

    /* Ignore broken pipe signals */
    signal(SIGPIPE, SIG_IGN);
    
    int client_fd = CLOSED;
    while (1/*client_fd == CLOSED*/)
    {
        client_fd = connect_socket(ip, port);
        if(client_fd!=CLOSED)
            break;
        sleep(5);
    }

    //epoll interface
    auto epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        close(client_fd);
        handle_error("epoll_create1", true);      
    }

    /* setup event for client socket*/
    struct epoll_event event;

    event.data.fd = client_fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP; /* edge-triggered read able to detect peer shutdown */

    /*  Add an entry to the interest list of the epoll file descriptor, fd_epoll */
    ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD ,client_fd, &event);
    if (ret == -1)
    {
        close(client_fd);
        close(epoll_fd);
        handle_error("epoll_ctl_client", true);      
    }

    /* initialize timer to write with determined interval */
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
	if (timer_fd == -1) 
    {
        close(client_fd);
		handle_error("timerfd_create", true);
	}

    struct itimerspec ts = {0};
    constexpr int interval = 10;
    constexpr int startup = 5;
    ts.it_interval.tv_sec = interval;
	ts.it_value.tv_sec = startup;

    if (timerfd_settime(timer_fd, TFD_TIMER_ABSTIME, &ts, NULL) == -1)
    {
        close(client_fd);
        close(timer_fd);
        close(epoll_fd);
		handle_error("timerfd_create", true);
    }

    /* clear before again usage */
    std::memset(&event, 0, sizeof(struct epoll_event));

    event.data.fd = timer_fd;
    event.events = EPOLLIN | EPOLLET; /* edge-triggered read */

    /*  Add an entry to the interest list of the epoll file descriptor, fd_epoll */
    ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD ,timer_fd, &event);
    if (ret == -1)
    {
        close(client_fd);
        close(timer_fd);
        close(epoll_fd);
        handle_error("epoll_ctl_timer", true);      
    }

    struct epoll_event *events;
    /* clean Buffer where events are returned */
    events = (struct epoll_event*)  calloc (MAXEVENTS, sizeof event);

    while (1)
    {    
        
        char buf[MAX_READ]={0};
        auto wait = epoll_wait(epoll_fd, events, MAXEVENTS, -1); //wait infinite time for events
        if (wait == -1)
        {
            close(client_fd);
            close(timer_fd);
            close(epoll_fd);
            handle_error("epoll_wait",true);      
        }
        for (size_t i = 0; i < wait; i++)
        {   

            std::cout<<"event: "<<events[i].events<<"\n";

            if (events[i].events & EPOLLRDHUP)
            {
                fprintf (stderr, "epoll error. events=%u\n", events[i].events);
	            close(events[i].data.fd);
                if (client_fd == events[i].data.fd)
                {
                    client_fd = CLOSED;
                }
                
	            continue;
            }
            else
            /* error or hang up happend */
            if ((events[i].events & EPOLLERR) ||
                (events[i].events & EPOLLHUP) ||
                (!(events[i].events & EPOLLIN)))
            {
                fprintf (stderr, "epoll error. events=%u\n", events[i].events);
	            close(events[i].data.fd);
                if (client_fd == events[i].data.fd)
                {
                    client_fd = CLOSED;
                }
                
	            continue;
            }
            else
            if (timer_fd == events[i].data.fd)
            {
                while (1)
                {
                    uint64_t expirations;
                    ret = read(timer_fd, &expirations, sizeof(uint64_t));
                    if (ret == -1)
                    {
                        if (errno != EAGAIN)
                        {
                            handle_error("read",true);
                        }
                        else
                        { 
                            // std::cout<<"another read call after timer expiration\n";
                            break;
                        }                     
                    }
                    else
                    if (ret != sizeof(uint64_t))
                    {
                        close(client_fd);
                        close(timer_fd);
                        close(epoll_fd);
                        handle_error("read_timer",true); 
                    }
                    else
                    {
                        std::cout<<expirations << " timer expiration\n";

                        std::string msg("Expiration: ");

                        if (client_fd != CLOSED)
                        {
                            ret = write(client_fd, msg.c_str(), msg.size());
                            if (ret == -1)
                            {
                                if (errno != EAGAIN)
                                {
                                    close(client_fd);
                                    std::cout<<client_fd<<"\n";
                                    handle_error("write");
                                } 
                            }                  
                            else
                            if (ret != msg.size())
                            {
                                std::cout<<"different amnt of written bytes: "<<ret <<",in comparison to msg: "<<sizeof(uint64_t)<<"\n";
                            }
                        }                        
                    }
                }  
            }            
            else
            {
              /* We have data on the fd waiting to be read. Read and
               * display it. We must read whatever data is available
               * completely, as we are running in edge-triggered mode
               * and won't get a notification again for the same
               * data. */
              auto drop_connection = false;
              int all_reads=0;
              while (1)
              {
                /* If errno == EAGAIN, that means we have read all
                * data. So go back to the main loop. */
                ret = read(events[i].data.fd,buf,sizeof buf ); 
                if (ret == -1)
                {
                    if (errno != EAGAIN)
                    {
                        handle_error("read");
                        drop_connection = true;
                    }
                    else
                    { 
                        std::cout<<"read all data:"<<all_reads << " bytes\n";
                    }
                    break;
                }
                else
                if (ret == 0)
                {
                  /* End of file. The remote has closed the
                  * connection. */
                  drop_connection = true;
                  break;
                }
                else
                {
                  if (buf[ret-1]=='\n')
                  {
                    buf[ret-1]='\0';
                  }
                  
                  std::cout<<"Received: "<<ret<<" bytes from fd: "<<events[i].data.fd<<"\n";
                  std::cout<<buf<<"\n";
                  all_reads+= ret;
                }
              }

              if (drop_connection)
              {
                std::cout<<"Dropped connection on descriptor: "<< events[i].data.fd << "\n";
                /* Closing the descriptor will make epoll remove it
                 * from the set of descriptors which are monitored. */
                close(events[i].data.fd);
                if (client_fd == events[i].data.fd)
                {
                    client_fd = CLOSED;
                }
              }
              
              
            }
        }

        while (client_fd == CLOSED)
        {
            sleep(5);
            client_fd = connect_socket(ip,port);
        }

        
    }
    
}