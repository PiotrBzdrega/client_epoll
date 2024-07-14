#pragma once
#include "ILog.h"

class Logger final : public ILog
{
    private:
        Logger(){ }
    public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    // void log(std::string_view) override {};
    inline static Logger& instance()
    {
       static Logger inst;
       return inst;
    };

};
