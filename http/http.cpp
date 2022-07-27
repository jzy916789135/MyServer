

#include "http.h"

#include <mysql/mysql.h>
#include <fstream>

//some information state
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";

//locker lock;
//map<string, string> users;

int setNonBlocking(int fd)
{
    int oldOption = fcntl(fd, F_GETFL);
    int newOption = oldOption | O_NONBLOCK;
    fcntl(fd, F_SETFL, newOption);
    return oldOption;
}

void addfd(int epollfd, int fd, bool oneShot)
{
    epoll_event event;
    event.data.fd = fd;

    event.events = EPOLLIN | EPOLLRDHUP;

    if(oneShot)
        event.events |= EPOLLONESHOT;
    
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setNonBlocking(fd);
}

void removefd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

//reset event EPOLLONESHOT
void modfd(int epollfd, int fd, int ev)
{
    epoll_event event;
    event.data.fd = fd;

    event.events = ev | EPOLLONESHOT | EPOLLRDHUP;

    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

int http::userCount = 0;
int http::hepollfd = -1;

void http::closeConn(bool closeflag)
{
    if(closeflag && (msockfd != -1))
    {
	removefd(hepollfd, msockfd);
	msockfd = -1;
	userCount--;
    }
}

void http::init(int sockfd, const sockaddr_in &addr, char *root)
{
    msockfd = sockfd;
    maddress = addr;

    addfd(hepollfd, sockfd, true);
    userCount++;

    mdocRoot = root;
    //mcloseLog = closelog;

    //strcpy(sqlUser, user.c_str());
    //strcpy(sqlPasswd, passwd.c_str());
    //strcpy(sqlName, sqlname,c_str());

    init();
}

void http::init()
{
    //mysql = NULL;
    bytesToSend = 0;
    bytesHaveSend = 0;
    mcheckState = CHECK_STATE_REQUESTLINE;
    mlinger = false;
    mmethod = GET;
    murl = 0;
    mversion = 0;
    mcontentLength = 0;
    mhost = 0;
    mstartLine = 0;
    mcheckedIdx = 0;
    mreadIdx = 0;
    mwriteIdx = 0;
    mcgi = 0;
    state = 0;
    timerFlag = 0;
    improv = 0;

    memset(mreadBuf, '\0', READ_BUFFER_SIZE);
    memset(mwriteBuf, '\0', WRITE_BUFFER_SIZE);
    memset(mfile, '\0', FILENAME_LEN);
}

http::LINE_STATUS http::parseLine()
{
    char tmp;
    for(; mcheckedIdx < mreadIdx; ++mcheckedIdx)
    {
        tmp = mreadBuf[mcheckedIdx];
	//Enter
	if(tmp == '\r')
	{
	    if((mcheckedIdx + 1) == mreadIdx)
	        return LINE_OPEN;
	    else if(mreadBuf[mcheckedIdx + 1] == '\n')
	    {
	        mreadBuf[mcheckedIdx++] = '\0';
		mreadBuf[mcheckedIdx++] = '\0';
		return LINE_OK;
	    }
	}

	else if(tmp == '\n')
	{
	    if(mcheckedIdx > 1 && mreadBuf[mcheckedIdx - 1] == '\r')
	    {
		mreadBuf[mcheckedIdx - 1] = '\0';
		mreadBuf[mcheckedIdx++]   = '\0';
	        return LINE_OK;
	    }
	}
    }
    return LINE_OPEN;
}

//Read the customer data circularly until no data is readable or the other party closes the connection
bool http::readOnce()
{
    if(mreadIdx >= READ_BUFFER_SIZE)
        return false;
    int bytesRead = 0;

    bytesRead = recv(msockfd, mreadBuf + mreadIdx, READ_BUFFER_SIZE - mreadIdx, 0);
    mreadIdx += bytesRead;

    if(bytesRead <= 0)
        return false;
   
    return true;
}

//parse http request line
http::HTTP_CODE http::parseRequestLine(char *text)
{
    murl = strpbrk(text, " \t");
    if(!murl)
        return BAD_REQUEST;
    //set this position as Terminator to get Previous characters
    *murl++ = '\0';
    char *method = text;
    if(strcasecmp(method, "GET") == 0)
        mmethod = GET;
    else if(strcasecmp(method, "POST") == 0)
    {
        mmethod = POST;
	mcgi = 1;
    }
    else
        return BAD_REQUEST;
    //Skip consecutive characters of spcae or "\t"
    murl += strspn(murl, " \t");
    
    mversion = strpbrk(murl, " \t");

    if(!mversion)
        return BAD_REQUEST;
    *mversion++ = '\0';
    mversion += strspn(mversion, " \t");

    if(strcasecmp(mversion, "HTTP/1.1") != 0)
        return BAD_REQUEST;
    if(strncasecmp(murl, "http://", 7) == 0)
    {
        murl += 7;
	murl = strchr(murl, '/');
    }
    if(strncasecmp(murl, "https://", 8) == 0)
    {
        murl += 8;
	murl = strchr(murl, '/');
    }

    if(!murl || murl[0] != '/')
        return BAD_REQUEST;
    if(strlen(murl) == 1)
        strcat(murl, "welcome.html");

    mcheckState = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

http::HTTP_CODE http::parseHeaders(char *text)
{
    if(text[0] == '\0')
    {
        if(mcontentLength != 0)
	{
	    mcheckState = CHECK_STATE_CONTENT;
	    return NO_REQUEST;
	}
	return GET_REQUEST;
    }

    else if(strncasecmp(text, "Connection:", 11) == 0)
    {
        text += 11;
	text += strspn(text, " \t");
        if(strcasecmp(text, "keep-alive") == 0)
	{
	    mlinger = true;
	}
    }
    
    else if(strncasecmp(text, "Content-length:", 15) == 0)
    {
        text += 15;
	text += strspn(text, " \t");
	mcontentLength = atol(text);
    }

    else if(strncasecmp(text, "Host:", 5) == 0)
    {
        text += 5;
	text += strspn(text, " \t");
	mhost = text;
    }

    else
    {
        LOG_INFO("oop!unknow header: %s", text);
    }

    return NO_REQUEST;
}

//Determine whether the HTTP request is read completely
http::HTTP_CODE http::parseContent(char *text)
{
    if(mreadIdx >= (mcontentLength + mcheckedIdx))
    {
        text[mcontentLength] = '\0';
        mstring = text;
	return GET_REQUEST;
    }
    return NO_REQUEST;
}

http::HTTP_CODE http::processRead()
{
    //to exit loop
    LINE_STATUS lineStatus = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = 0;
    while((mcheckState == CHECK_STATE_CONTENT && lineStatus == LINE_OK)|| ((lineStatus = parseLine()) == LINE_OK))
    {
	text = getLine();
	mstartLine = mcheckedIdx;
        LOG_INFO("%s", text);
	switch(mcheckState)
	{
	    //mcheckState has been set CHECK_STATE_REQUESTLINE first in init();
	    case CHECK_STATE_REQUESTLINE:
	    {
	        
	        ret = parseRequestLine(text);
		if(ret == BAD_REQUEST)
		{
		    return BAD_REQUEST;
		}    
		break;
	    }  
	    case CHECK_STATE_HEADER:
	    {
	        ret = parseHeaders(text);
		if(ret == BAD_REQUEST)
		{
		    return BAD_REQUEST;
		} 
		else if(ret == GET_REQUEST)
		{
		    return doRequest();
		} 

		break;
	    }
	    case CHECK_STATE_CONTENT:
	    {
	        ret = parseContent(text);
		if(ret == GET_REQUEST)
		    return doRequest();
		lineStatus = LINE_OPEN;
		break;
	    }
	    default:
	        return INTERNAL_ERROR;
	}
    }

    return NO_REQUEST;
}

http::HTTP_CODE http::doRequest()
{
    strcpy(mfile, mdocRoot);
    int len = strlen(mdocRoot);

    const char *p = strrchr(murl, '/');

    if (*(p + 1) == '5')
    {
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/picture.html");
        strncpy(mfile + len, m_url_real, strlen(m_url_real));
        free(m_url_real);
    }
    else if (*(p + 1) == '6')
    {
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/video.html");
        strncpy(mfile+ len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else if (*(p + 1) == '7')
    {
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/fans.html");
        strncpy(mfile + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else
        strncpy(mfile + len, murl, FILENAME_LEN - len - 1);


    if(stat(mfile, &mfileStat) < 0)
        return NO_RESOURCE;

    if(!(mfileStat.st_mode & S_IROTH))
        return FORBIDDEN_REQUEST;

    if(S_ISDIR(mfileStat.st_mode))
        return BAD_REQUEST;

    int fd = open(mfile, O_RDONLY);
    mfileAddress = (char *)mmap(0, mfileStat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    return FILE_REQUEST;
}

void http::unmap()
{
    if(mfileAddress)
    {
        munmap(mfileAddress, mfileStat.st_size);
	mfileAddress = 0;
    }
}

bool http::write()
{
    int temp = 0;
    if(bytesToSend == 0)
    {
        modfd(hepollfd, msockfd, EPOLLIN);
	init();
	return true;
    }
    while(1)
    {
        temp = writev(msockfd, miv, mivCount);
	if(temp < 0)
	{
	    //buffer full
	    if(errno == EAGAIN)
	    {
	        modfd(hepollfd, msockfd, EPOLLOUT);
		return true;
	    }
	    //send fail, but not buffer full
	    unmap();
	    return false;
	}

	bytesHaveSend += temp;
	bytesToSend -= temp;
	//first iov has been send
	if(bytesHaveSend >= miv[0].iov_len)
	{
	   miv[0].iov_len = 0;
	   miv[1].iov_base = mfileAddress + (bytesHaveSend - mwriteIdx);
	   miv[1].iov_len = bytesToSend;
	}
	else
	{
	    miv[0].iov_base = mwriteBuf + bytesHaveSend;
	    miv[0].iov_len = miv[0].iov_len - bytesHaveSend;
	}

	if(bytesToSend <= 0)
	{
	    unmap();
	    modfd(hepollfd, msockfd, EPOLLIN);

	    if(mlinger)
	    {
	        init();
		return true;
	    }
	    else
	    {
	        return false;
	    }
	}
    }
}

bool http::addResponse(const char *format, ...)
{
    if(mwriteIdx >= WRITE_BUFFER_SIZE)
    {
        return false;
    }

    va_list arg;
    va_start(arg, format);
    int len = vsnprintf(mwriteBuf + mwriteIdx, WRITE_BUFFER_SIZE - 1 - mwriteIdx, format, arg);
    if(len >= (WRITE_BUFFER_SIZE -1 - mwriteIdx))
    {
        va_end(arg);
	return false;
    }
    mwriteIdx += len;
    va_end(arg);

    LOG_INFO("request:%s", mwriteBuf);
    return true;
}

bool http::addStatusLine(int status, const char *title)
{
    return addResponse("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool http::addHeaders(int contentLen)
{
    return addContentLength(contentLen) && addLinger() && addBlankLine();
}

bool http::addContentLength(int contentLen)
{
    return addResponse("Content-Length:%d\r\n", contentLen);
}

bool http::addContentType()
{
    return addResponse("Content-Type:%s\r\n", "text/html");
}

bool http::addLinger()
{
    return addResponse("Connection:%s\r\n", (mlinger == true) ? "keep-alive" : "close");
}

bool http::addBlankLine()
{
    return addResponse("%s", "\r\n");
}

bool http::addContent(const char *content)
{
    return addResponse("%s", content);
}

bool http::processWrite(HTTP_CODE ret)
{

    switch(ret)
    {
        case INTERNAL_ERROR:
	{
	    addStatusLine(500, error_500_title);
	    addHeaders(strlen(error_500_form));
	    if(!addContent(error_500_title))
	        return false;
	    break;
	}

	case BAD_REQUEST:
	{
	    addStatusLine(404, error_404_title);
	    addHeaders(strlen(error_404_form));
	    bool ret = addContent(error_404_form);
	    if(!ret)
	    {
	        return false;
	    }

	    break;
	}

	case FORBIDDEN_REQUEST:
	{
	    addStatusLine(403, error_403_title);
	    addHeaders(strlen(error_403_form));
	    if(!addContent(error_403_form));
	        return false;
	    break;
	}

	case FILE_REQUEST:
	{
	    addStatusLine(200, ok_200_title);
	    if(mfileStat.st_size != 0)
	    {
	        addHeaders(mfileStat.st_size);
		miv[0].iov_base = mwriteBuf;
		miv[0].iov_len = mwriteIdx;
		miv[1].iov_base = mfileAddress;
		miv[1].iov_len = mfileStat.st_size;
		mivCount = 2;
		bytesToSend = mwriteIdx + mfileStat.st_size;
		return true;
	    }
	    else
	    {
	        const char *ok_string = "<html><body></body></html>";
		addHeaders(strlen(ok_string));
		if(!addContent(ok_string))
		    return false;
	    }
	}
	default:
	{
	    return false;
	}

    }

    miv[0].iov_base = mwriteBuf;
    miv[0].iov_len = mwriteIdx;
    mivCount = 1;
    bytesToSend = mwriteIdx;
    return true;
}

void http::process()
{
    HTTP_CODE readRet = processRead();
    if(readRet == NO_REQUEST)
    {
        modfd(hepollfd, msockfd, EPOLLIN);
	return;
    }
    bool writeRet = processWrite(readRet);
    if(!writeRet)
    {
        closeConn();
    }
    modfd(hepollfd, msockfd, EPOLLOUT);

}




