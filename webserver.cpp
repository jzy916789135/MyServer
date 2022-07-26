
#include"webserver.h"
#include<iostream>


using namespace std;

webServer::webServer()
{
    // init http class object
    users = new http[MAX_FD];

    // root folder path
    char serverPath[200];
    getcwd(serverPath, 200);
    char tmpRoot[6] = "/root";
    root = (char *)malloc(strlen(serverPath) + strlen(tmpRoot) + 1);
    strcpy(root, serverPath);
    strcat(root, tmpRoot);

    // TImer
    userTimer = new clientData[MAX_FD];
}

webServer::~webServer()
{
    close(epollfd);
    close(listenfd);
    close(pipefd[0]);
    close(pipefd[1]);
    delete []users;
    delete []userTimer;
    delete pool;
}

void webServer::init(int _port, int _threadNum)
{
    port = _port;
    threadNum = _threadNum;
}

void webServer::initThreadpool()
{
    pool = new threadpool<http>(threadNum);
}

void webServer::eventListen()
{
    listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    //close connection gracefully
    {
        struct linger tmp = {1,1};
	setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }

    //about address
    int ret = 0;
    struct sockaddr_in serverAddress;
    bzero(&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(port);

    int flag = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    ret = bind(listenfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    assert(ret >= 0);
    //set socket to listen model
    ret = listen(listenfd, 5);
    assert(ret >= 0);

    utils.init(TIMESLOT);
    //about epoll create
    epoll_event events[MAX_EVENT_NUMBER];
    epollfd = epoll_create(5);
    assert(epollfd != -1);
    
    utils.addfd(epollfd, listenfd, false);
    http::hepollfd = epollfd;

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
    assert(ret != -1);
    utils.setNonBlocking(pipefd[1]);
    utils.addfd(epollfd, pipefd[0], false);

    //add sig
    utils.addsig(SIGPIPE, SIG_IGN);
    utils.addsig(SIGALRM, utils.sigHandler, false);
    utils.addsig(SIGTERM, utils.sigHandler, false);
    
    alarm(TIMESLOT);

    Utils::upipefd = pipefd;
    Utils::uepollfd = epollfd;
}

void webServer::timer(int connfd, sockaddr_in _clientAddress)
{
    users[connfd].init(connfd, _clientAddress, root);

    // init clientData
    userTimer[connfd].address = _clientAddress;
    userTimer[connfd].sockfd = connfd;
    utilTimer *timer = new utilTimer;
    timer->userData = &userTimer[connfd];
    timer->cb_func = cb_func;
    time_t cur = time(NULL);
    timer->expire = cur + 3 * TIMESLOT;
    userTimer[connfd].timer = timer;
    utils.timerList.addTimer(timer);
}

// if there is data transmission, delay the timer by 3 units and adjust the position
void webServer::adjustTimer(utilTimer *timer)
{
   time_t cur = time(NULL);
   timer->expire = cur + 3 * TIMESLOT;
   utils.timerList.adjustTimer(timer);
}

// delete timer
void webServer::delTimer(utilTimer *timer, int sockfd)
{
    // close sockfd
    timer->cb_func(&userTimer[sockfd]);
    if(timer)
    {
        utils.timerList.delTimer(timer);
    }
}

bool webServer::dealNewConnection()
{
    struct sockaddr_in clientAddress;
    socklen_t clientAddrLen = sizeof(clientAddress);

    int connfd = accept(listenfd, (struct sockaddr *)&clientAddress, &clientAddrLen);
    if(connfd < 0)
    {
        std::cout << " accept error " << std::endl;
        return false;
    }
    if(http::userCount >= MAX_FD)
    {
        std::cout << " Inernal server busy " << std::endl;
        return false;
    }
    timer(connfd, clientAddress);

    return true;
}

bool webServer::dealSignal(bool &timeout, bool &stopServer)
{
    int ret = 0;
    int sig;
    char signals[1024];
    ret = recv(pipefd[0], signals, sizeof(signals), 0);
    if(ret == -1)
        return false;
    else if(ret == 0)
        return false;
    else
    {
        for(int i = 0; i < ret; i++)
        {
            switch(signals[i])
	    {
	        case SIGALRM:
	        {
	            timeout = true;
		    break;
	        }
	        case SIGTERM:
	        {
	            stopServer = true;
		    break;
	        }
	    }
        }
   }
   return true;
}

void webServer::dealRead(int sockfd)
{
    utilTimer *timer = userTimer[sockfd].timer;
    if(users[sockfd].readOnce())
    {
        pool->append(users + sockfd, 0);
	if(timer)
	{
	    adjustTimer(timer);
	}
    }
    else
    {
        delTimer(timer,sockfd);
    }
}

void webServer::dealWrite(int sockfd)
{
    utilTimer *timer = userTimer[sockfd].timer;
    if(users[sockfd].write())
    {
        if(timer)
	    adjustTimer(timer);
    }
    else
        delTimer(timer, sockfd);
}

void webServer::eventLoop()
{
    bool timeout = false;
    bool stopServer = false;

    while(!stopServer)
    {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if(number < 0 && errno != EINTR)
	{
	    break;
	}
	for(int i = 0; i < number; i++)
	{
	    
	    int sockfd = events[i].data.fd;
	    // deal new connection
	    if(sockfd == listenfd)
	    {
		//std::cout << " new"  << std::endl;
		bool flag = dealNewConnection();
		if(flag == false)
		    continue;
	    }
	    
	    // close connection
	    else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
	    {
		// close connection, and remove timer
		utilTimer *timer = userTimer[sockfd].timer;
		delTimer(timer, sockfd);
	    }

	    // deal sig
	    else if((sockfd == pipefd[0]) && (events[i].events & EPOLLIN))
	    {
		//std::cout << " deal sig"  << std::endl;
	        bool flag = dealSignal(timeout, stopServer);
		if(flag == false)
		    std::cout << " dealSig failure " << std::endl;
	    }
	    
	    // deal data from user connection
	    else if(events[i].events & EPOLLIN)
	    {
		//std::cout << " deal read"  << std::endl;
	        dealRead(sockfd);
	    }
	    else if(events[i].events & EPOLLOUT)
	    {
		//std::cout << " deal write"  << std::endl;
	        dealWrite(sockfd);
	    }

	}

	if(timeout)
	{
	    utils.timerHandler();

	    timeout = false;
	}
    }
}






