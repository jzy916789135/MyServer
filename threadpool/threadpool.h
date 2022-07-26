#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<iostream>
#include<list>
#include<cstdio>
#include<exception>
#include<pthread.h>
#include"../locker.h"

template <typename T>
class threadpool
{
public:
    threadpool(int threadNumber = 8, int maxRequest = 10000);
    ~threadpool();

    bool append(T* request, int state);

private:
    static void *worker(void *arg);
    void run();

private:
    int mthreadNumber;          //thread number in threadpoo

    int mmaxRequests;           //max requests allowed in request queue

    pthread_t *mthreads;        //array of discribe threadpool

    std::list<T*> mworkQueue;    //request queue

    locker mqueueLocker;

    sem mqueueState;

};

template <typename T>
threadpool<T>::threadpool(int threadNumber, int maxRequest)
{

    mthreads = NULL;
    if(threadNumber <= 0 || maxRequest <= 0)
        throw std::exception();
    mthreads = new pthread_t[threadNumber];
    if(!mthreads)
        throw std::exception();
    for(int i = 0; i < threadNumber; ++i)
    {
        if(pthread_create(mthreads + i, NULL, worker, this) != 0)
	{
	    delete[] mthreads;
	    throw std::exception();
	}

        if(pthread_detach(mthreads[i]))
        {
            delete[] mthreads;
	    throw std::exception();
	}

    }
    mthreadNumber = threadNumber;
    mmaxRequests = maxRequest;
    std::cout << "threadpool has been created " << std::endl;
}

template <typename T>
threadpool<T>::~threadpool()
{
    delete[] mthreads;
}

template <typename T>
bool threadpool<T>::append(T* request, int state)
{
    mqueueLocker.lock();
    if(mworkQueue.size() >= mmaxRequests)
    {
        mqueueLocker.unlock();
	return false;
    }

    request->state = state;
    mworkQueue.push_back(request);
    mqueueLocker.unlock();
    mqueueState.post();

    return true;
}


template <typename T>
void *threadpool<T>::worker(void *arg)
{
    threadpool *pool = (threadpool*)arg;
    pool->run();
    return pool;
}

template <typename T>
void threadpool<T>::run()
{
    while(true)
    {
        mqueueState.wait();
	mqueueLocker.lock();
	if(mworkQueue.empty())
	{
	    mqueueLocker.unlock();
	    continue;
	}

	T* request = mworkQueue.front();
	mworkQueue.pop_front();
	mqueueLocker.unlock();
	if(!request)
	{
	    continue;
	}
	request->process();
    }

}
#endif
