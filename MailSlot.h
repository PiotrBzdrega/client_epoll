#pragma once

#include <winsock2.h>
#include <windows.h>
#include <functional>
#include <string>
#include <thread>

namespace com
{
     enum class userLevel{server, client};
     class MailSlot
     {
     private:
          HANDLE _handle{nullptr};
          const wchar_t* _ms_name;
          std::function<HANDLE(const wchar_t *ms_name)> initCall;
          std::thread read_thr;
          int init();
          void read();
     public:
          MailSlot(userLevel lvl, const wchar_t *ms_name);
          ~MailSlot();
          void readMail();
          int write(const std::string &req); //TODO: check, maybe std::string_view will be better
     };
}



