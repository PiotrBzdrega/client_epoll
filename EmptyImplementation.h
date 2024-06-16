#pragma once

#include "ILog.h"
#include "IDB.h"

namespace
{
    class EmptyLogger final : public ILog
    {
        // private:
        //     EmptyLogger() { }
        // public:
        // void log(std::string_view) override {};
        // EmptyLogger(const EmptyLogger& obj) = delete; // Delete copy constructor
    };

    class EmptyDB final : public IDB
    {
        // private:
        //     EmptyDB() { }
        public:
        bool set(int,int) override { return false;}
        bool get(int,int&) override { return false;}
        // EmptyDB(const EmptyDB& obj) = delete; // Delete copy constructor
    };
    
    /* inline will be treated as a single instance across all translation units.*/
    inline EmptyLogger emptyLogger;
    inline EmptyDB emptyDB;
}
