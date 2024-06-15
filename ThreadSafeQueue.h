#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>
#include <memory>

namespace COM
{
    template<typename T>
    class ThreadSafeQueue
    {
    private:
        mutable std::mutex mtx; /*Since locking a mutex is a mutating operation, the mutex object must be marked mutable*/
        std::deque<T> que;
        std::condition_variable cv;
    public:
        ThreadSafeQueue() {};
        ~ThreadSafeQueue() {};
        void push(const T &new_value)
        {
            /* lock function scope */
            std::lock_guard<std::mutex> lck(mtx); 

            /* push new value at the end of que if exclusive access available */
            que.push_back(new_value);
        }
        void push_front(const T &new_value)
        {
            /* lock function scope */
            std::lock_guard<std::mutex> lck(mtx);

            /* push new value at the first position if exclusive access available */
            que.push_front(new_value);
        }
        std::shared_ptr<T> pop()
        {   
            /* lock function scope */
            std::unique_lock<std::mutex> lck(mtx);

            /* block till queue has new data */
            cv.wait(lck,[this]{return !que.empty();});

            /* obtain last element (added as first) from queue */
            std::shared_ptr<T> res(std::make_shared<T>(que.front()));

            /* remove last element (added as first) from queue */
            que.pop_front();

            return res;
        }
        void clear()
        {
            /* lock function scope */
            std::lock_guard<std::mutex> lck(mtx); 

            //Erases all elements from the container.
            que.clear();
        }
    };
}


