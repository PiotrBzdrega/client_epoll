#pragma once

class ILog
{
private:
    /* data */
public:
    virtual void log() = 0;
    virtual ~ILog() = default;
};