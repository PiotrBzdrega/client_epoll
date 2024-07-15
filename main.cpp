#include <iostream>
#include <string>
#include <cstring> // std::memset
#include <algorithm> // std::replace
#include <charconv> //std::from_chars

#ifdef  __linux__
#include <sys/socket.h> //connect, bind
#include <unistd.h> //close, read, write
#include <sys/file.h> //flock, open
#include <getopt.h> //getopt_long
#include <arpa/inet.h> // inet_ntop
#include <sys/timerfd.h> // timerfd
#include <sys/epoll.h> //epoll
#include <signal.h> //signal
#include <mqueue.h> //mq
#elif _WIN32 //available for both x64 and x32
#include "MailSlot.h"
#include "WinSockInit.h"
#include "ProcessInstance.h"
#endif

#include "Logger.h"
#include "Auxiliary.h"
#include "EndPoint.h"


/* container to transfer command lines from second instance to EndPoint */
COM::ThreadSafeQueue<std::string> queue;

#ifdef  __linux__

constexpr auto mq_name="/mq_client";

void new_msg_queue(union sigval sv)
{

    mqd_t mqdes = *(static_cast<mqd_t*>(sv.sival_ptr));

    struct mq_attr attr;
    /* Determine max. msg size; allocate buffer to receive msg */
    if (mq_getattr(mqdes, &attr) == -1)
    {
        handle_error("mq_getattr");
    }    
    
    //std::printf("Attributes of MQ %s \n",mq_name.c_str());
    //std::printf("Maximum message size: %ld bytes\n", attr.mq_msgsize);
    //std::printf("Maximum number of messages: %ld\n", attr.mq_maxmsg);

    /* Re-register notification , mq_notify is only one-shot event*/

    struct sigevent sev;
    sev.sigev_notify = SIGEV_THREAD; //Deliver via thread creation
    sev.sigev_notify_function = new_msg_queue; // thread function 
    sev.sigev_notify_attributes = NULL;
    sev.sigev_value.sival_ptr = sv.sival_ptr;   /* Arg. to thread func. */
    if (mq_notify(mqdes, &sev) == -1)
    {
        handle_error("mq_notify");
    }

    char *buf;
    buf = (char*)malloc(attr.mq_msgsize); //Maximum message size
    if (buf == NULL)
    {
        handle_error("malloc");
    }
        
    //received byte count
    ssize_t rcv = mq_receive(mqdes, buf, attr.mq_msgsize, NULL);
    if (rcv == -1)
    {
        handle_error("mq_receive");
    }

    /* returned array could not have null terminator*/
    buf[rcv]='\0';

    /* even if buffer is released (free()), malloc can allocate same area in next msg; there is no \0 character at the end, so rcv len must be took into account for format %.*s*/
    std::printf("Read %zd bytes from MQ %s: %s\n", rcv, mq_name, buf);
    // int ret = std::strcmp("exit",buf);

    /* push new data into queue */
    queue.push(buf);
    free(buf);
    // if (ret == 0)
    // {
    //     exit(EXIT_SUCCESS);         /* Terminate the process */
    // }
}
#elif _WIN32 //available for both x64 and x32 




#endif




int main(int argc, char *argv[])
{

#ifdef _WIN32 //available for both x64 and x32
    /* WinsockInit singleton RAII wrapper */
    auto& winsock = WinSock::instance();

    /* Child or Parent process */
    COM::ProcessInstance isParent;
#endif

    


    /* Logger singlton*/
    auto& logger = Logger::instance(); //TODO: find best design 
    // use logger from miq

    int ret;
    std::string_view ip;
    uint16_t port;
    bool with_tls = false;
    bool test_timer = false;

    extract to class posix message queue from main
    mqd_t mqdes;
    /* create if do not exist */
    int pid_file = open("/var/run/client.pid", O_CREAT | O_RDWR, 0666);
    if (pid_file == -1) {
        handle_error("open",true);
    }
/*The fcntl() system call provides record
locking, allowing processes to place multiple read and write locks on different
regions of the same file.*/

    struct flock fl;

    // Initialize the flock structure
    fl.l_type = F_WRLCK;   // Exclusive lock
    fl.l_whence = SEEK_SET; //Seek from beginning of file
    fl.l_start = 0;
    fl.l_len = 0; //whole file if 0

    // Attempt to acquire the lock
    int rc = fcntl(pid_file, F_SETLK, &fl); //Set record locking info (non-blocking)
    
    if(rc) 
    {
        /* Another instance, file already locked*/

        std::cout<<"errno: "<<strerror(errno)<<"\n";
        if(EWOULDBLOCK == errno)
        {
            std::cout<<"\033[0;91m another instance is running \033[0m\n";

            int rc = fcntl(pid_file, F_GETLK, &fl); //Get record locking info
            std::cout<<"\033[0;91m Process ID of the process that holds the blocking lock "<<fl.l_pid<<"\033[0m\n";       
        }
        else
        {
            exit(EXIT_FAILURE);
        }

        std::printf("Read arguments:\n");
        for (size_t i = 0; i < argc; i++)
        {
           std::printf("%s\t",argv[i]);
        }
        std::printf("\n");
        
        if (argc<2)
        {
            std::printf("Missing valid arguments\n");
            exit(EXIT_FAILURE);
        }
        
        auto begin = argv[1];
        auto size_last_arg = strlen(argv[argc-1]);
        auto end = argv[argc-1];
        end += size_last_arg;
        std::string buffer(begin,end-begin);
        std::replace(buffer.begin(),buffer.end(),'\0',' ');

        std::string_view request(buffer);


        mailSlot



        // check if we pass correctly without spaces between -i 12532u5032:23423
        // Open the message queue for sending
        mqdes = mq_open(mq_name, O_WRONLY);
        if (mqdes == (mqd_t)-1) {
            handle_error("mq_open",true);
        }
        
        std::printf("sizeof(%s) = %ld\n",buffer.data(), strlen(buffer.data()));

        // if (mq_send(mqdes, argv[argc-1], strlen(argv[argc-1]), 0) == -1) 
        if (mq_send(mqdes, buffer.data(), buffer.size(), 0) == -1) 
        {
            handle_error("mq_send",true);
        }

        exit(EXIT_SUCCESS);
    }
    else
    {
        /* First Instance */


        mqdes = mq_open(mq_name,O_RDONLY | O_CREAT | O_NONBLOCK /* | O_EXCL */,0600,NULL);
        if (mqdes == (mqd_t) -1)
        {
            handle_error("mq_open",true);
        }

        struct sigevent sev;
        sev.sigev_notify = SIGEV_THREAD; //Deliver via thread creation
        sev.sigev_notify_function = new_msg_queue; // thread function 
        sev.sigev_notify_attributes = NULL;
        sev.sigev_value.sival_ptr = &mqdes;   /* Arg. to thread func. */

        /* register a notification request for the message queue */
        if (mq_notify(mqdes, &sev) == -1)
        {
            handle_error("mq_notify",true);
        }

        /* clean mq in case of previous remains ,notification won't be rised in case of non-empty queue*/
        struct mq_attr attr;
        if (mq_getattr(mqdes, &attr) == -1)
        {
            handle_error("mq_getattr",true);
        }    

        char *emp_buf;
        emp_buf = (char*)malloc(attr.mq_msgsize); //Maximum message size
        if (emp_buf == NULL)
        {
            handle_error("malloc",true);
        }
        while(mq_receive(mqdes, emp_buf, attr.mq_msgsize, 0) >= 0)
        {
          std::cout << "empty the mq " << emp_buf << "\n";
        }
    }

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
          std::from_chars(optarg,optarg+ std::strlen(optarg),port);
        //   port = optarg;
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
    
    /* create client instance */
    COM::EndPoint client(ip,port,queue);

    // while (!client.connect())
    // {
    //     sleep(2); // try again after a while
    // }
   

    // //epoll interface
    // auto epoll_fd = epoll_create1(0);
    // if (epoll_fd == -1)
    // {
    //     closeFd(client());
    //     handle_error("epoll_create1", true);      
    // }

    // /* setup event for client socket*/
    // struct epoll_event event;

    // event.data.fd = client();
    // event.events = EPOLLIN | EPOLLET  | EPOLLRDHUP; /* edge-triggered read able to detect peer shutdown */

    // /*  Add an entry to the interest list of the epoll file descriptor, fd_epoll */
    // ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD ,client(), &event);
    // if (ret == -1)
    // {
    //     client.close();
    //     closeFd(epoll_fd);
    //     handle_error("epoll_ctl_client", true);      
    // }

    // /* initialize timer to write with determined interval */
    // int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
	// if (timer_fd == -1) 
    // {
    //     client.close();
    //     closeFd(epoll_fd);
	// 	handle_error("timerfd_create", true);
	// }

    // struct itimerspec ts = {0};
    // int interval = test_timer ? 10 : 0; 
    // int startup = test_timer ? 5 : 0;  
    // ts.it_interval.tv_sec = interval;
	// ts.it_value.tv_sec = startup;

    // if (timerfd_settime(timer_fd, TFD_TIMER_ABSTIME, &ts, NULL) == -1)
    // {
    //     client.close();
    //     closeFd(timer_fd);
    //     closeFd(epoll_fd);
	// 	handle_error("timerfd_create", true);
    // }

    // /* clear before again usage */
    // std::memset(&event, 0, sizeof(struct epoll_event));

    // event.data.fd = timer_fd;
    // event.events = EPOLLIN | EPOLLET; /* edge-triggered read */

    // /*  Add an entry to the interest list of the epoll file descriptor, fd_epoll */
    // ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD ,timer_fd, &event);
    // if (ret == -1)
    // {
    //     client.close();
    //     closeFd(timer_fd);
    //     closeFd(epoll_fd);
    //     handle_error("epoll_ctl_timer", true);      
    // }

    // struct epoll_event *events;
    // /* clean Buffer where events are returned */
    // events = (struct epoll_event*)  calloc (MAXEVENTS, sizeof event);

    // while (1)
    // {    
    //     char buf[MAX_READ]={0};
    //     auto wait = epoll_wait(epoll_fd, events, MAXEVENTS, -1); //wait infinite time for events
    //     if (wait == -1)
    //     {
    //         client.close();
    //         closeFd(timer_fd);
    //         closeFd(epoll_fd);
    //         handle_error("epoll_wait",true);      
    //     }
    //     for (size_t i = 0; i < wait; i++)
    //     {   

    //         std::printf("epoll event: %u\n",events[i].events);

    //         //TODO: search from list of EndPoint's to find events[i].data.fd, if not in list then different type fd

    //         /* error or hang up happend */
    //         if ((events[i].events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) && 
    //             (!(events[i].events & EPOLLIN))) /* TODO: some servers sends msg then close connection an 8193 is obtained should i close and  */
    //         {
    //             fprintf (stderr, "epoll error. events=%u\n", events[i].events); //TODO: event string

    //             if (events[i].data.fd == client())
    //             {
    //                 client.close();
    //             }
    //             else
    //             {
    //                 closeFd(events[i].data.fd);
    //             }
                
                
	//             continue;
    //         }
    //         else
    //         if (timer_fd == events[i].data.fd)
    //         {
    //             while (1)
    //             {
    //                 uint64_t expirations;
    //                 ret = read(timer_fd, &expirations, sizeof(uint64_t));
    //                 if (ret == -1)
    //                 {
    //                     if (errno != EAGAIN)
    //                     {
    //                         handle_error("read",true);
    //                     }
    //                     else
    //                     { 
    //                         // std::cout<<"another read call after timer expiration\n";
    //                         break;
    //                     }                     
    //                 }
    //                 else
    //                 if (ret != sizeof(uint64_t))
    //                 {
    //                     client.close();
    //                     closeFd(timer_fd);
    //                     closeFd(epoll_fd);
    //                     handle_error("read_timer",true); 
    //                 }
    //                 else
    //                 {
    //                     std::cout<<expirations << " timer expiration\n";

    //                     std::string msg("Expiration: ");

    //                     if (client() != CLOSED)
    //                     {
    //                         ret = write(client(), msg.c_str(), msg.size());
    //                         if (ret == -1)
    //                         {
    //                             if (errno != EAGAIN)
    //                             {
    //                                 client.close();
    //                                 handle_error("write");
    //                             } 
    //                         }                  
    //                         else
    //                         if (ret != msg.size())
    //                         {
    //                             std::cout<<"different amnt of written bytes: "<<ret <<",in comparison to msg: "<<sizeof(uint64_t)<<"\n";
    //                         }
    //                     }                        
    //                 }
    //             }  
    //         }            
    //         else
    //         {
    //           /* We have data on the fd waiting to be read. Read and
    //            * display it. We must read whatever data is available
    //            * completely, as we are running in edge-triggered mode
    //            * and won't get a notification again for the same
    //            * data. */

    //             if (events[i].data.fd != client())
    //             {
    //                 throw std::runtime_error("Only client should appears here");
    //             }
              

    //           /* connection has been closed */
    //           auto drop_connection = (events[i].events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) ? true : false;
    //           int all_reads=0;
    //           while (1)
    //           {
                
    //             ret = client.read(buf,sizeof buf); 
    //             if (ret == -1)
    //             {   
    //                 /* If errno == EAGAIN, that means we have read all
    //                 data. So go back to the main loop. */
    //                 std::printf("not more data available, read %u bytes\n",all_reads);
    //                 break;
    //             }           
    //             else
    //             if (ret == 0)
    //             {
    //                 /* End of file. The remote has closed the
    //                 * connection. */
    //                 //OR
    //                 /* Error during reading */
    //                 drop_connection = true;
    //                 break;
    //             }
    //             else
    //             {
    //                 /* Successful read*/

    //                 /* TODO: why I put it here */
    //                 if (buf[ret-1]=='\n')
    //                 {
    //                   buf[ret-1]='\0';
    //                 }
                  
    //                 std::cout<<"Received: "<<ret<<" bytes from fd: "<<client()<<"\n";
    //                 std::printf("%s\n",buf);
                    
    //                 // break;
                      
    //               all_reads+= ret;
    //             }
    //           }

    //             if (drop_connection)
    //             {
    //                 std::cout<<"Dropped connection on fd: "<< client()<< "\n";
    //                 /* Closing the descriptor will make epoll remove it
    //                  * from the set of descriptors which are monitored. */
    //                 client.close();
    //             }
              
              
    //         }
    //     }

    //     while (client() == CLOSED)
    //     {
    //         sleep(5);
    //         client.connect();

    //         if(client() == CLOSED)
    //         {continue;}


    //         /* clear before again usage */
    //         std::memset(&event, 0, sizeof(struct epoll_event));

    //         event.data.fd = client();
    //         event.events = EPOLLIN | EPOLLET  | EPOLLRDHUP; /* edge-triggered read able to detect peer shutdown */

    //         /*  Add an entry to the interest list of the epoll file descriptor, fd_epoll */
    //         ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD ,client(), &event);
    //         if (ret == -1)
    //         {
    //             client.close();
    //             closeFd(epoll_fd);
    //             handle_error("epoll_ctl_client", true);      
    //         }
    //     }

        
    // }
    // while(1) //TODO: join instead superloop
    // {
    //     sleep(100000);
    // }

    /* wait Manager to be finnished */
    client.join();
}