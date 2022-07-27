#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "../locker.h"

using namespace std;

template<class T>
class blockQueue
{
public:
    blockQueue(int maxSize = 1000)
    {
        if(maxSize <= 0)
	{
	    exit(-1);
	}
	mmaxSize = maxSize;
	marray = new T[mmaxSize];
	msize = 0;
	mfront = -1;
	mback = -1;
    }

    void clear()
    {
        mmutex.lock();
	msize = 0;
	mfront = -1;
	mback = -1;
	mmutex.unlock();
    }

    ~blockQueue()
    {
        mmutex.lock();
        if(marray != NULL)
	    delete [] marray;
        mmutex.unlock();
    }    

    //judge full
    bool full()
    {
        mmutex.lock();
	if(msize >= mmaxSize)
	{
	    mmutex.unlock();
	    return true;
	}
	mmutex.unlock();
	return false;
    }

    //judge empty
    bool empty()
    {
        mmutex.lock();
	if(msize == 0)
	{
	    mmutex.unlock();
	    return true;
	}
	mmutex.unlock();
	return false;
    }

    //return front
    bool front(T &value)
    {
        mmutex.lock();
	if(msize == 0)
	{
	    mmutex.unlock();
	    return false;
	}
	value = marray[mfront];
	mmutex.unlock();
	return true;
    }

    //return back
    bool back(T &value)
    {
        mmutex.lock();
	if(msize == 0)
	{
	    mmutex.unlock();
	    return false;
	}
	value = marray[mback];
	mmutex.unlock();
	return true;
    }

    int size() 
    {
        int tmp = 0;
        mmutex.lock();
        tmp = msize;   
        mmutex.unlock();
	return tmp;
    }

    int maxSize()
    {
        int tmp = 0;
	mmutex.lock();
	tmp = mmaxSize;
	mmutex.unlock();
	return tmp;
    }

    //push element to array
    bool push(const T &item)
    {
	
        mmutex.lock();
	if(msize >= mmaxSize)
	{
	    mcond.broadcast();
	    mmutex.unlock();
	    return false;
	}

	mback = (mback + 1) % mmaxSize;
	marray[mback] = item;

	msize++;

	mcond.broadcast();
	mmutex.unlock();
        
	return true;
    }

    //pop element
    bool pop(T &item)
    {
        mmutex.lock();
	while(msize <= 0)
	{
	    if(!mcond.wait(mmutex.get()))
	    {
	        mmutex.unlock();
		return false;
	    }
	}

	mfront = (mfront + 1) % mmaxSize;
	item = marray[mfront];

	msize--;
	mmutex.unlock();
	return true;
    }

private:
    locker mmutex;
    cond mcond;

    T *marray;
    int msize;
    int mmaxSize;
    int mfront;
    int mback;

    
};



#endif
