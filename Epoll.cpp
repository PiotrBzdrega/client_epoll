#include "Epoll.h"
#include "Auxiliary.h"

namespace COM
{

    // modify Server and then check if epoll correctly returns notifications, make progress with libnodave each type
    Epoll::Epoll(ThreadSafeQueue<std::string> &queue)
    {
        _fd = epoll_create1(0);
        if (_fd == -1)
        {
            handle_error("epoll_create1");
            return;    
        }

        /* start off waiter thread*/
        waiter = std::thread(&Epoll::waiterFunction,this,std::ref(queue)); //Directly passing queue will attempt to copy it
    }
    Epoll::~Epoll()
    {
        if (waiter.joinable())
        {
            waiter.join();
        }
    }

    bool Epoll::addNew(int fd, uint32_t param)
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

    int Epoll::waiterFunction(ThreadSafeQueue<std::string> &queue)
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
                if (errno==EINTR)
                {
                    /* signal occured */
                    continue;
                }

                handle_error("epoll_wait",true);
                continue;     
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