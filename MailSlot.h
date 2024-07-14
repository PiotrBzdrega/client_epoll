#pragma once

#include <winsock2.h>
#include <windows.h>
#include <functional>
#include <string>
#include <thread>
#include "ThreadSafeQueue.h"

namespace com
{
     constexpr auto ms_default_name=TEXT("\\\\.\\mailslot\\ms_instance");
     constexpr auto READ_BUFFER_SIZE = 2048;
     enum class userLevel{server, client};
     class MailSlot
     {
     private:
          HANDLE _handle{nullptr};
          const wchar_t* _ms_name;
          userLevel _lvl;
          std::function<HANDLE(const wchar_t *ms_name)> initCall;
          std::thread read_thr;
          int init();
          void read(COM::ThreadSafeQueue<std::string> &queue);
          
     public:
          MailSlot(userLevel lvl, const wchar_t* ms_name = ms_default_name);
          ~MailSlot();
          void join();
          void readMail(COM::ThreadSafeQueue<std::string> &queue);
          int write(const std::string &message); //TODO: check, maybe std::string_view will be better
     };
}



