#ifndef HTTP_H
#define HTTP_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>
#include <iostream>

#include "../locker.h"
#include "../timer/timer.h"

using namespace std;

class http
{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;

    enum METHOD
    {
        GET = 0,
	POST,
	HEAD,
	PUT,
	DELETE,
	TRACE,
	OPTIONS,
	CONNECT,
	PATH
    };

    enum CHECK_STATE
    {
	CHECK_STATE_REQUESTLINE = 0,
	CHECK_STATE_HEADER,
	CHECK_STATE_CONTENT
    };

    enum HTTP_CODE
    {
	NO_REQUEST,
	GET_REQUEST,
	BAD_REQUEST,
	NO_RESOURCE,
	FORBIDDEN_REQUEST,
	FILE_REQUEST,
	INTERNAL_ERROR,
	CLOSED_CONNECTION
    };

    enum LINE_STATUS
    {
        LINE_OK = 0,
	LINE_BAD,
	LINE_OPEN
    };


public:
    http() {}
    ~http() {}

public:
    void init(int sockfd, const sockaddr_in &addr, char *root);

    void closeConn(bool closeflag = true);
    void process();
    //read all information once
    bool readOnce();
    bool write();

    sockaddr_in *getAddress()
    {
        return &maddress;
    }
    //void initmysqlResult();
    int timerFlag;
    int improv;

private:
    void init();
    HTTP_CODE processRead();
    bool processWrite(HTTP_CODE ret);
    HTTP_CODE parseRequestLine(char *text);
    HTTP_CODE parseHeaders(char *text);
    HTTP_CODE parseContent(char *text);
    HTTP_CODE doRequest();
    char *getLine() {return mreadBuf + mstartLine;};

    LINE_STATUS parseLine();
    void unmap();
    bool addResponse(const char *format, ...);
    bool addStatusLine(int status, const char *title);
    bool addHeaders(int contentLen);
    bool addContentLength(int contentLen);
    bool addContent(const char *content);
    bool addContentType();
    bool addLinger();
    bool addBlankLine();
public:
    static int hepollfd;
    static int userCount;
    //MYSQL *mysql;
    int state;

private:
    int msockfd;
    sockaddr_in maddress;
    char mreadBuf[READ_BUFFER_SIZE];
    int mreadIdx;
    int mcheckedIdx;
    int mstartLine;

    char mwriteBuf[WRITE_BUFFER_SIZE];
    int mwriteIdx;

    CHECK_STATE mcheckState;
    METHOD mmethod;

    char mfile[FILENAME_LEN];
    char *murl;
    char *mversion;
    char *mhost;

    int mcontentLength;
    bool mlinger;
    char *mfileAddress;

    struct stat mfileStat;
    struct iovec miv[2];
    int mivCount;

    int bytesToSend;
    int bytesHaveSend;

    int mcgi;
    char *mstring;
    char *mdocRoot;

    map<string, string> musers;
    int mcloseLog;

    char sqlUser[100];
    char sqlPasswd[100];
    char sqlName[100];
};

#endif















