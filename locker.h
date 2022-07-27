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

class cond
{
public:
    cond()
    {
        if(pthread_cond_init(&mcond, NULL) != 0)
        {
            throw std::exception();
        }
    }
    ~cond()
    {
	pthread_cond_destroy(&mcond);
    }

    bool wait(pthread_mutex_t *mutex)
    {
	int ret = 0;
	ret = pthread_cond_wait(&mcond, mutex);
	return ret == 0;
    }
    bool signal()
    {
        return pthread_cond_signal(&mcond) == 0;
    }
    bool broadcast()
    {
        return pthread_cond_broadcast(&mcond) == 0;
    }
private:
    pthread_cond_t mcond;
};

#endif
