#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "TcpServer.h"
/*
路径：/home/kobe/linux/dabing/luffy

gcc main.c Buffer.c Channel.c ChannelMap.c EpollDispatcher.c EventLoop.c HttpRequest.c Httpresponse.c TcpConnection.c TcpServer.c ThreadPool.c WorkerThread.c SelectDispatcher.c PollDispatcher.c -lpthread

./a.out

192.168.146.129:10000
*/

int main(int argc, char *argv[])
{
#if 0
    if (argc < 3)
    { 
        printf("./a.out port path\n");
        return -1;
    }
    unsigned short port = atoi(argv[1]);
    // 切换服务器的工作路径
    chdir(argv[2]);
#else
    unsigned short port = 10000;
    chdir("/home/kobe/linux/dabing/luffy");
#endif
    // 启动服务器
    struct TcpServer *server = tcpServerInit(port, 4);
    tcpServerRun(server);

    return 0;
}