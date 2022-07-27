#include<iostream>
#include"webserver.h"
#include<unistd.h>
#include<string>
#include<vector>
#include<stdlib.h>

using namespace std;


int main(int argc, char *argv[])
{

    int port = 9006;
    int threadNum = 8;
    int logflag = 0;
    int logwritetype = 1; 
    webServer server;

    server.init(port, threadNum, logflag, logwritetype);

    server.initLog();

    server.initThreadpool();

    server.eventListen();

    server.eventLoop();

    return 0;
}
