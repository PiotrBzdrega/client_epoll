#include "MailSlot.h"

#include "MailSlotImpl.h"
#include <cstdio>
#include <stdexcept>

namespace com
{
    MailSlot::MailSlot(userLevel lvl, const wchar_t *ms_name) : _ms_name(ms_name)
    {
        if (!ms_name)
        {
            throw std::invalid_argument("not valid MailSlot name");
        }

        initCall =  (lvl == userLevel::client) ? initReceiver : initSender;
    }

    MailSlot::~MailSlot()
    {
        if (_handle)
        {
            CloseHandle(_handle);
        }
    }

    int MailSlot::init()
    {
        _handle=initCall(_ms_name);
        GetMailslotInfo to insert
        return _handle ? GetLastError() : EXIT_SUCCESS;     
    }

    void readMail()
    {
        
        read_thr=std::thread(&MailSlot::read,this);
    }

    void MailSlot::read()
    {
        if(init())
        {
            //error occurs
            return;
        }
        while (1)
        {
            /* code */
        }
        
    }

    int MailSlot::write(const std::string &req)
    {
        return 0;
    }
}

