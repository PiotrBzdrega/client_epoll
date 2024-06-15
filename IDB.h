#pragma once

class IDB
{
private:
    /* data */
public:
    virtual bool set(int address, int value) = 0;
    virtual bool get(int address, int &var) = 0;
    virtual ~IDB() = default;
};
