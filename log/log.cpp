#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include "log.h"
#include <pthread.h>

using namespace std;

log::log()
{
    mCount = 0;
    misaysnc = false;
}

log::~log()
{
    if(mfp != NULL)
    {
        fclose(mfp);
    }
}

bool log::init(const char *fileName, int closeLog, int bufferSize, int splitLines, int maxQueuesize)
{

    //if maxQueuesize > 0, set method of write async
    if(maxQueuesize >= 1)
    {
        misaysnc = true;
	mlogQueue = new blockQueue<string>(maxQueuesize);
	
	//cteate write thread
	pthread_t writeThread;
	pthread_create(&writeThread, NULL, threadWriteLog, NULL);
    }

    mcloseLog = closeLog;
    mbufferSize = bufferSize;
    mbuffer = new char[mbufferSize];
    memset(mbuffer, '\0', mbufferSize);
    msplitLines = splitLines;

    time_t t = time(NULL);
    struct tm *sys = localtime(&t);
    struct tm myTime = *sys;

    const char *p = strrchr(fileName, '/');
    char logFullName[256] = {0};
    if(p == NULL)
    {
	snprintf(logFullName, 255, "%d_%02d_%02d_%s", myTime.tm_year + 1900, myTime.tm_mon + 1, myTime.tm_mday, fileName);
    }
    else
    {
        strcpy(mlogname, p + 1);
	strncpy(mdirname, fileName, p - fileName + 1);
	snprintf(logFullName, 255, "%s%d_%02d_%02d_%s", mdirname, myTime.tm_year + 1900, myTime.tm_mon + 1, myTime.tm_mday, mlogname);
    }

    mtoday = myTime.tm_mday;
    mfp = fopen(logFullName, "a");
    if(mfp == NULL)
    { 
        return false;
    }
    return true;
}

void log::writeLog(int type, const char *format, ...)
{
    struct timeval now = {0,0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm *sys = localtime(&t);
    struct tm myTime = *sys;
    char typestr[16] = {0};
    
    switch(type)
    {
    case 0:
        strcpy(typestr, "[DEBUG]:");
	break;
    case 1:
        strcpy(typestr, "[INFO]:");
	break;
    case 2:
        strcpy(typestr, "[WARN]:");
	break;
    case 3:
        strcpy(typestr, "[ERROr]:");
	break;
    default:
        strcpy(typestr, "[INFO]:");
	break;
    }
    //write a log
    mmutex.lock();
    mCount++;
    if(mtoday != myTime.tm_mday || mCount % msplitLines == 0)
    {
        char newLog[256] = {0};
	fflush(mfp);
	fclose(mfp);
	char tail[16] = {0};

	snprintf(tail, 16, "%d_%02d_%02d_", myTime.tm_year + 1900, myTime.tm_mon + 1, myTime.tm_mday);
    
        if(mtoday != myTime.tm_mday)
	{
	    snprintf(newLog, 255, "%s%s%s", mdirname, tail, mlogname);
	    mtoday = myTime.tm_mday;
	    mCount = 0;
	}
	else
	{
	    snprintf(newLog, 255, "%s%s%s.%lld", mdirname, tail, mlogname, mCount / msplitLines);
	}	
	mfp = fopen(newLog, "a");
    }

    mmutex.unlock();
    
    va_list valst;
    va_start(valst, format);

    string logStr;
    mmutex.lock();

    //write content;
    int n  = snprintf(mbuffer, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ", myTime.tm_year + 1900, myTime.tm_mon + 1, myTime.tm_mday,(myTime.tm_hour + 15 ) % 24, myTime.tm_min, myTime.tm_sec, now.tv_usec, typestr);

    int m = vsnprintf(mbuffer + n, mbufferSize - 1 - n, format, valst);
    mbuffer[n + m] = '\n';
    mbuffer[n + m + 1] = '\0';
    logStr = mbuffer;

    mmutex.unlock();
    if(misaysnc && !mlogQueue->full())
    {
        mlogQueue->push(logStr);
    }
    else
    {
        mmutex.lock();
	fputs(logStr.c_str(), mfp);
	mmutex.unlock();
    }
    va_end(valst);
}
void log::flush(void)
{
    mmutex.lock();
    fflush(mfp);
    mmutex.unlock();
}
