#ifndef __SAFEQUEUE_H__
#define __SAFEQUEUE_H__

#include <list>
#include <boost/thread.hpp>

template <typename T>
struct SafeQueue
{
    void Produce(const T& t)
    {
        boost::mutex::scoped_lock lock(mtx);
        list.push_back(T());
    }

    bool Consume(T& t)
    {
        boost::mutex::scoped_lock lock(mtx);
        if (list.empty())
            return false;
        t = list.front();
        list.pop_front();
        return true;
    }

private:
    typedef std::list<T> TList;
    TList list;    
private:
    boost::mutex mtx;
};

#endif
