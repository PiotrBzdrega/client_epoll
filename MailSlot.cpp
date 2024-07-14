#include "MailSlot.h"

#include "MailSlotImpl.h"
#include <cstdio>
#include <stdexcept>
#include <array>
#include "NotImplemented.h"

namespace com
{
    MailSlot::MailSlot(userLevel lvl, const wchar_t *ms_name) : _ms_name(ms_name), _lvl(lvl)
    {
        if (!ms_name)
        {
            throw std::invalid_argument("not valid MailSlot name");
        }

        initCall =  (lvl == userLevel::client) ? initReceiver : initSender;
    }

    MailSlot::~MailSlot()
    {
        join();
    }

    void MailSlot::join()
    {
        if(read_thr.joinable())
        {
            read_thr.join();
        }

        if (_handle)
        {
            CloseHandle(_handle);
        }
    }

    int MailSlot::init()
    {
        /* init already executed */
        if(_handle)
        {
            return EXIT_SUCCESS;
        }
        /* call user dependent functional init */
        _handle=initCall(_ms_name);
        
        /* return error code in case of failure */
        return _handle ? EXIT_SUCCESS : GetLastError();     
    }

    void MailSlot::readMail(COM::ThreadSafeQueue<std::string> &queue)
    {
        if (_lvl == userLevel::server)
        {
            throw NotImplemented();
        }
        
        /* create thread with function member*/
        read_thr=std::thread(&MailSlot::read,this,queue);
    }

    void MailSlot::read(COM::ThreadSafeQueue<std::string> &queue)
    {
        if(init())
        {
            //error occurs
            return;
        }

        /* The maximum message size, in bytes, allowed for this mailslot. */
        DWORD maxMessageSize; 
        /* The size of the next message, in bytes; MAILSLOT_NO_MESSAGE ((DWORD)-1) There is no next message. */
        DWORD nextSize;
        /* The total number of messages waiting to be read, when the function returns */
        DWORD messageCount;
        /* The amount of time, in milliseconds, a read operation can wait for a message to be written to the mailslot before a time-out occurs */
        DWORD readTimeout;
        /* A pointer to the variable that receives the number of bytes read when using a synchronous hFile parameter */
        DWORD bytesRead;

        /* Retrieves information about the specified mailslot */
        if(!GetMailslotInfo(_handle,&maxMessageSize,&nextSize,&messageCount,&messageCount))
        {
            //error occurs
            return;
        }

        /* not valid */
        if (maxMessageSize<=0)
        {
            /* code */
        }

        /* buffer for ReadFile */
        std::array<char,READ_BUFFER_SIZE> buffer;


        while (1)
        {
            if(ReadFile(_handle, &buffer, buffer.size(), &bytesRead, nullptr))
            {
                // Null-terminate the string
                buffer[bytesRead]='\0'; 
                /* pass arguments to queue */
                queue.push(std::string(buffer.data(),bytesRead));
            }
            else
            {
                return;
            }
        }
        
    }

    int MailSlot::write(const std::string &message)
    {
        if (_lvl==userLevel::client)
        {
            throw NotImplemented();
        }
        
        if(init())
        {
            //error occurs
            return;
        }
        DWORD bytesWritten;
        if (!WriteFile(_handle, message.c_str(), message.size(), &bytesWritten, nullptr)) 
        {
            return GetLastError();
        }
        return 0;
    }
}

