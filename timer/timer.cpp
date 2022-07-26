#include"timer.h"
#include<iostream>
#include"../http/http.h"

using namespace std;

sortTimerList::sortTimerList()
{
    head = NULL;
    tail = NULL;
    FixedTimeout = 5;
}

sortTimerList::~sortTimerList()
{
    utilTimer *tmp = head;
    while(tmp)
    {
        head = tmp->next;
	delete tmp;
	tmp = head;
    }
}

void sortTimerList::addTimer(utilTimer *timer)
{
    if(!timer)
    {
	return;
    }
    if(!head)
    {
        head = tail = timer;
        return;
    }
    if(timer->expire < head->expire)
    {
        timer->next = head;
	head->prev = timer;
	head = timer;
	return;
    }
    addTimer(timer, head);
}

void sortTimerList::adjustTimer(utilTimer* timer)
{
    if(!timer)
        return;

    utilTimer *tmp = timer->next;
    if(!tmp || (timer->expire < tmp->expire))
        return;

    if(timer == head)
    {
        head = head->next;
	head->prev = NULL;
	timer->next = NULL;
	addTimer(timer, head);
    }
    else
    {
        timer->prev->next = timer->next;
	timer->next->prev = timer->prev;
	addTimer(timer, timer->next);
    }
}

void sortTimerList::delTimer(utilTimer* timer)
{
    if(!timer)
        return;
    if((timer == head) && (head == tail))
    {
        delete timer;
	head = NULL;
	tail = NULL;
	return;
    }
    if(timer == head)
    {
        head = head->next;
	head->prev = NULL;
	delete timer;
	return;
    }
    if(timer == tail)
    {
	tail = tail->prev;
	tail->next = NULL;
	delete timer;
	return;
    }

    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}

int sortTimerList::tick()
{
    if(!head)
        return FixedTimeout;

    time_t cur = time(NULL);
    utilTimer *tmp = head;
    std::cout << " tick begin() " << std::endl;
    while(tmp)
    {
        if(cur < tmp->expire)
	{
	    return (tmp->expire - cur);
	}
	tmp->cb_func(tmp->userData);
	head = tmp->next;
	if(head)
	    head->prev = NULL;
	delete tmp;
	tmp = head;
    }
    return FixedTimeout;
}

void sortTimerList::addTimer(utilTimer *timer, utilTimer* lsthead)
{
    utilTimer *prev = lsthead;
    utilTimer *tmp = prev->next;
    while(tmp)
    {
        if(timer->expire < tmp->expire)
	{
	    prev->next = timer;
	    timer->next = tmp;
	    tmp->prev = timer;
	    timer->prev = prev;
	    break;
	}
	prev = tmp;
	tmp = tmp->next;
    }
    if(!tmp)
    {
        prev->next = timer;
	timer->prev = prev;
	timer->next = NULL;
	tail = timer;
    }
}

void Utils::init(int timeSlot)
{
    TIMESLOT = timeSlot;
}

int Utils::setNonBlocking(int fd)
{
    int oldOption = fcntl(fd, F_GETFL);
    int newOption = oldOption | O_NONBLOCK;
    fcntl(fd, F_SETFL, newOption);
    return oldOption;
}

//Kernel event table register read event
void Utils::addfd(int epollfd, int fd, bool oneShot)
{
    epoll_event event;
    event.data.fd = fd;

    event.events = EPOLLIN | EPOLLRDHUP;

    if(oneShot)
        event.events |= EPOLLONESHOT;

    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setNonBlocking(fd);
}

//Signal processing function
void Utils::sigHandler(int sig)
{
    int saveErrno = errno;
    int msg = sig;
    send(upipefd[1], (char*)&msg, 1, 0);
    errno = saveErrno;
}

//Set signal function
void Utils::addsig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    // SA_RESTART   When a blocked system call is executed, the process will not return when receiving the signal, but will execute the system call again
    if(restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

//Timed processing task, timed trigger SIGALARM signal
void Utils::timerHandler()
{
    int tmpTimeSlot = timerList.tick();
    TIMESLOT = tmpTimeSlot;
    alarm(TIMESLOT);
}

int *Utils::upipefd = 0;
int Utils::uepollfd = 0;

class Utils;
void cb_func(clientData* userData)
{
    epoll_ctl(Utils::uepollfd, EPOLL_CTL_DEL, userData->sockfd, 0);
    assert(userData);
    close(userData->sockfd);
    http::userCount--;
}
