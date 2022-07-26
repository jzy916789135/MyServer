#ifndef LOCKER_H
#define LOCKER_H

#include<exception>
#include<pthread.h>
#include<semaphore.h>

class sem
{
public:
    sem()
    {
        if(sem_init(&msem, 0, 0) != 0)
	    throw std::exception();
    }
    sem(int num)
    {
        if(sem_init(&msem, 0, num) != 0)
	    throw std::exception();
    }
    ~sem()
    {
        sem_destroy(&msem);
    }

    bool wait()
    {
        return sem_wait(&msem) == 0;
    }

    bool post()
    {
        return sem_post(&msem) == 0;
    }

private:
    sem_t msem;
};

class locker
{
public:
    locker()
    {
	if(pthread_mutex_init(&mmutex, NULL) != 0)
	    throw std::exception();
    }
    ~locker()
    {
        pthread_mutex_destroy(&mmutex);
    }

    bool lock()
    {
        return pthread_mutex_lock(&mmutex) == 0;
    }

    bool unlock()
    {
	return pthread_mutex_unlock(&mmutex) == 0;
    }

    pthread_mutex_t *get()
    {
	return &mmutex;
    }

private:
    pthread_mutex_t mmutex;
};

#endif
