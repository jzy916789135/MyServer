#include<iostream>
#include"webserver.h"
#include<unistd.h>
#include<string>
#include<vector>
#include<stdlib.h>

using namespace std;



class test
{
public:
    test() {mstate = 0; testnum = 0;}
    ~test() {}
    void show();

    int mstate;
    int testnum;
    char name[10];
};


int main(int argc, char *argv[])
{
    int port = 9006;
    int threadNum = 8;
    webServer server;

    server.init(port, threadNum);

    server.initThreadpool();

    server.eventListen();

    server.eventLoop();

    return 0;

}
