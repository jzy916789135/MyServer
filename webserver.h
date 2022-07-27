#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "./threadpool/threadpool.h"
#include "./http/http.h"

const int MAX_FD = 65536;
const int MAX_EVENT_NUMBER = 10000;
const int TIMESLOT = 5;

class webServer
{
public:
    webServer();
    ~webServer();

    void init(int _port, int _threadNum, int _logFlag, int _logWriteType);

    void initThreadpool();
    void initLog();
    void eventListen();
    void eventLoop();

    void timer(int connfd, struct sockaddr_in _clientAddress);
    void adjustTimer(utilTimer *timer);
    void delTimer(utilTimer *timer, int sockfd);
    bool dealNewConnection();
    bool dealSignal(bool &timeout, bool &stopServer); 
    void dealRead(int sockfd);
    void dealWrite(int sockfd);    

public:

    // basic:
    int port;
    char* root;

    int epollfd;
    int pipefd[2];
    http* users;

    int logFlag;
    int logWriteType;
    // about threadpoll
    threadpool<http> *pool;
    int threadNum;

    // about epoll_event
    epoll_event events[MAX_EVENT_NUMBER];
    int listenfd;
    
    // about timer
    clientData *userTimer;
    Utils utils;
};


#endif
