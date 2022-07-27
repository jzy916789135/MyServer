#ifndef LOG_H
#define LOG_H

#include<stdio.h>
#include<iostream>
#include<string>
#include<stdarg.h>
#include<pthread.h>

#include"blockQueue.h"

using namespace std;

class log
{
public:

    static log *getInstance()
    {
        static log instance;
	return &instance;
    }

    //Write thread write log
    static void *threadWriteLog(void *args)
    {
    	log::getInstance()->asyncWriteLog();
    }

    bool init(const char *filename, int closeLog, int bufferSize = 8192, int splitLines = 500000, int maxQueueSize = 0);

    void writeLog(int type, const char *format, ...);

    void flush(void);

    int getflag() { std::cout << mcloseLog << std::endl; return mcloseLog;};
private:

    log();
    virtual ~log();
    void *asyncWriteLog()
    {
        string singleLog;
	//get a log string form logqueue, and write it to file
	while(mlogQueue->pop(singleLog))
	{
	    mmutex.lock();
	    fputs(singleLog.c_str(), mfp);
	    mmutex.unlock();
	}
    }

private:

    char mdirname[128]; //dir name

    char mlogname[128]; //log name

    int msplitLines;    //max lines

    int mbufferSize;    //buffer size

    long long mCount;   //lines recorded

    int mtoday;

    char *mbuffer;

    FILE *mfp;          //pointer to log file

    blockQueue<string> *mlogQueue;  //block queue

    bool misaysnc;      //flag of aysnc

    locker mmutex;      //locker

    int mcloseLog;       //close log if(0 == log::getInstance()->getflag())

};

#define LOG_DEBUG(format, ...)  {log::getInstance()->writeLog(0, format, ##__VA_ARGS__); log::getInstance()->flush();}
#define LOG_INFO(format, ...)  { log::getInstance()->writeLog(1, format, ##__VA_ARGS__); log::getInstance()->flush();}
#define LOG_WARN(format, ...)  {log::getInstacne()->writeLog(2, format, ##__VA_ARGS__); log::getInstance()->flush();}
#define LOG_ERROR(format, ...)  {log::getInstance()->writeLog(3, format, ##__VA_ARGS__); log::getInstance()->flush();}

#endif
