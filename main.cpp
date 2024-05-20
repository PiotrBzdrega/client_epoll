#include <iostream>
#include <string>
#include <cstring> // std::memset

#include <sys/socket.h> //connect, bind
#include <unistd.h> //close, read, write
#include <sys/file.h> //flock, open
#include <getopt.h> //getopt_long
#include <arpa/inet.h> // inet_ntop
#include <sys/timerfd.h> // timerfd
#include <sys/epoll.h> //epoll
#include <signal.h> //signal

#include "EndPoint.h"

constexpr auto MAXEVENTS = 64;
constexpr int MAX_READ = 2048;

int main(int argc, char *argv[])
{
    int ret;
    std::string_view ip, port;
    bool with_tls = false;
    bool test_timer = false;

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
            {"port",    required_argument, 0, 'p' },
            {"ssl",     no_argument,       0,  1 },
            {"timer",   no_argument,       0,  2 },
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
        case 1:
            printf ("option %s\n", long_options[option_index].name);
            with_tls = true;
            break;
        case 2:
            printf ("option %s\n", long_options[option_index].name);
            test_timer = true;
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

    /* Ignore broken pipe signal , 
    if this is not assigned , epoll_wait leave with undefined behavior when pipe closes abruptly */
    signal(SIGPIPE, SIG_IGN);
    
    COM::EndPoint client(ip,port,with_tls);


    while (!client.connectIO())
    {
        sleep(2); // try again after a while
    }
   

    //epoll interface
    auto epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        closeFd(client());
        handle_error("epoll_create1", true);      
    }

    /* setup event for client socket*/
    struct epoll_event event;

    event.data.fd = client();
    event.events = EPOLLIN | EPOLLET  | EPOLLRDHUP; /* edge-triggered read able to detect peer shutdown */

    /*  Add an entry to the interest list of the epoll file descriptor, fd_epoll */
    ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD ,client(), &event);
    if (ret == -1)
    {
        client.closeIO();
        closeFd(epoll_fd);
        handle_error("epoll_ctl_client", true);      
    }

    /* initialize timer to write with determined interval */
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
	if (timer_fd == -1) 
    {
        client.closeIO();
        closeFd(epoll_fd);
		handle_error("timerfd_create", true);
	}

    struct itimerspec ts = {0};
    int interval = test_timer ? 10 : 0; 
    int startup = test_timer ? 5 : 0;  
    ts.it_interval.tv_sec = interval;
	ts.it_value.tv_sec = startup;

    if (timerfd_settime(timer_fd, TFD_TIMER_ABSTIME, &ts, NULL) == -1)
    {
        client.closeIO();
        closeFd(timer_fd);
        closeFd(epoll_fd);
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
        client.closeIO();
        closeFd(timer_fd);
        closeFd(epoll_fd);
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
            client.closeIO();
            closeFd(timer_fd);
            closeFd(epoll_fd);
            handle_error("epoll_wait",true);      
        }
        for (size_t i = 0; i < wait; i++)
        {   

            std::printf("epoll event: %u\n",events[i].events);

            //TODO: search from list of EndPoint's to find events[i].data.fd, if not in list then different type fd

            /* error or hang up happend */
            if ((events[i].events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) && 
                (!(events[i].events & EPOLLIN))) /* TODO: some servers sends msg then close connection an 8193 is obtained should i close and  */
            {
                fprintf (stderr, "epoll error. events=%u\n", events[i].events); //TODO: event string

                if (events[i].data.fd == client())
                {
                    client.closeIO();
                }
                else
                {
                    closeFd(events[i].data.fd);
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
                        client.closeIO();
                        closeFd(timer_fd);
                        closeFd(epoll_fd);
                        handle_error("read_timer",true); 
                    }
                    else
                    {
                        std::cout<<expirations << " timer expiration\n";

                        std::string msg("Expiration: ");

                        if (client() != CLOSED)
                        {
                            ret = write(client(), msg.c_str(), msg.size());
                            if (ret == -1)
                            {
                                if (errno != EAGAIN)
                                {
                                    client.closeIO();
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

                if (events[i].data.fd != client())
                {
                    throw std::runtime_error("Only client should appears here");
                }
              

              /* connection has been closed */
              auto drop_connection = (events[i].events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) ? true : false;
              int all_reads=0;
              while (1)
              {
                
                ret = client.read(buf,sizeof buf); 
                if (ret == -1)
                {   
                    /* If errno == EAGAIN, that means we have read all
                    data. So go back to the main loop. */
                    std::printf("not more data available, read %u bytes\n",all_reads);
                    break;
                }           
                else
                if (ret == 0)
                {
                    /* End of file. The remote has closed the
                    * connection. */
                    //OR
                    /* Error during reading */
                    drop_connection = true;
                    break;
                }
                else
                {
                    /* Successful read*/

                    /* TODO: why I put it here */
                    if (buf[ret-1]=='\n')
                    {
                      buf[ret-1]='\0';
                    }
                  
                    std::cout<<"Received: "<<ret<<" bytes from fd: "<<client()<<"\n";
                    std::printf("%s\n",buf);
                    
                    // break;
                      
                  all_reads+= ret;
                }
              }

                if (drop_connection)
                {
                    std::cout<<"Dropped connection on fd: "<< client()<< "\n";
                    /* Closing the descriptor will make epoll remove it
                     * from the set of descriptors which are monitored. */
                    client.closeIO();
                }
              
              
            }
        }

        while (client() == CLOSED)
        {
            sleep(5);
            client.connectIO();

            if(client() == CLOSED)
            {continue;}


            /* clear before again usage */
            std::memset(&event, 0, sizeof(struct epoll_event));

            event.data.fd = client();
            event.events = EPOLLIN | EPOLLET  | EPOLLRDHUP; /* edge-triggered read able to detect peer shutdown */

            /*  Add an entry to the interest list of the epoll file descriptor, fd_epoll */
            ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD ,client(), &event);
            if (ret == -1)
            {
                client.closeIO();
                closeFd(epoll_fd);
                handle_error("epoll_ctl_client", true);      
            }
        }

        
    }
    
}