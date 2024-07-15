#include "EventLoop.h"  // 包含自定义的 EventLoop 头文件
#include <assert.h>     // 包含断言头文件，用于条件检查
#include <sys/socket.h> // 包含套接字头文件
#include <unistd.h>     // 包含 POSIX 操作系统 API，如 close, read, write
#include <stdlib.h>     // 包含标准库函数，如 malloc, free
#include <stdio.h>      // 标准输入输出库，用于 perror, printf 等函数
#include <string.h>     // 字符串处理函数，如 strcpy, strlen

// 初始化事件循环，调用带线程名参数的初始化函数
struct EventLoop *eventLoopInit()
{
    return eventLoopInitEx(NULL);
}

// 写数据到本地通信的文件描述符
void taskWakeup(struct EventLoop *evLoop)
{
    const char *msg = "我是要成为海贼王的男人!!!";  // 要写入的信息
    write(evLoop->socketPair[0], msg, strlen(msg)); // 将消息写入 socketPair[0]
}

// 读本地通信的文件描述符中的数据
int readLocalMessage(void *arg)
{
    struct EventLoop *evLoop = (struct EventLoop *)arg;
    char buf[256];                                 // 缓冲区
    read(evLoop->socketPair[1], buf, sizeof(buf)); // 从 socketPair[1] 中读取数据
    return 0;
}

// 带线程名参数的初始化事件循环
struct EventLoop *eventLoopInitEx(const char *threadName)
{
    struct EventLoop *evLoop = (struct EventLoop *)malloc(sizeof(struct EventLoop)); // 分配事件循环结构体的内存
    evLoop->isQuit = false;                                                          // 初始化 isQuit 标志
    evLoop->threadID = pthread_self();                                               // 获取当前线程 ID
    pthread_mutex_init(&evLoop->mutex, NULL);                                        // 初始化互斥锁
    strcpy(evLoop->threadName, threadName == NULL ? "MainThread" : threadName);      // 设置线程名
    evLoop->dispatcher = &SelectDispatcher;                                          // 初始化 dispatcher
    evLoop->dispatcherData = evLoop->dispatcher->init();                             // 初始化 dispatcher 数据
    // 初始化任务队列
    evLoop->head = evLoop->tail = NULL;
    // 初始化 channelMap
    evLoop->channelMap = channelMapInit(128);
    // 创建本地通信的 socketpair
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, evLoop->socketPair);
    if (ret == -1)
    {
        perror("socketpair");
        exit(0);
    }
    // 指定规则: socketPair[0] 发送数据, socketPair[1] 接收数据
    struct Channel *channel = channelInit(evLoop->socketPair[1], ReadEvent, readLocalMessage, NULL, NULL, evLoop);
    // 将 channel 添加到任务队列
    eventLoopAddTask(evLoop, channel, ADD);

    return evLoop;
}

// 启动事件循环
int eventLoopRun(struct EventLoop *evLoop)
{
    assert(evLoop != NULL); // 检查事件循环是否为空
    // 获取事件分发和检测模型
    struct Dispatcher *dispatcher = evLoop->dispatcher;
    // 检查线程 ID 是否匹配
    if (evLoop->threadID != pthread_self())
    {
        return -1;
    }
    // 循环处理事件
    while (!evLoop->isQuit)
    {
        dispatcher->dispatch(evLoop, 2); // 调用 dispatch 函数，超时时长 2 秒
        eventLoopProcessTask(evLoop);    // 处理任务队列中的任务
    }
    return 0;
}

// 激活事件，处理被激活的文件描述符
int eventActivate(struct EventLoop *evLoop, int fd, int event)
{
    if (fd < 0 || evLoop == NULL)
    {
        return -1;
    }
    // 获取 fd 对应的 channel
    struct Channel *channel = evLoop->channelMap->list[fd];
    assert(channel->fd == fd); // 检查 channel 的 fd 是否匹配
    // 处理读事件
    if (event & ReadEvent && channel->readCallback)
    {
        channel->readCallback(channel->arg);
    }
    // 处理写事件
    if (event & WriteEvent && channel->writeCallback)
    {
        channel->writeCallback(channel->arg);
    }
    return 0;
}

// 添加任务到任务队列
int eventLoopAddTask(struct EventLoop *evLoop, struct Channel *channel, int type)
{
    pthread_mutex_lock(&evLoop->mutex); // 加锁，保护共享资源
    // 创建新的任务节点
    struct ChannelElement *node = (struct ChannelElement *)malloc(sizeof(struct ChannelElement));
    node->channel = channel;
    node->type = type;
    node->next = NULL;
    // 如果任务队列为空
    if (evLoop->head == NULL)
    {
        evLoop->head = evLoop->tail = node;
    }
    else
    {
        evLoop->tail->next = node; // 将新节点添加到队列末尾
        evLoop->tail = node;       // 更新队列尾指针
    }
    pthread_mutex_unlock(&evLoop->mutex); // 解锁
    // 处理节点
    /*
     * 细节:
     *   1. 对于链表节点的添加: 可能是当前线程也可能是其他线程(主线程)
     *       1). 修改 fd 的事件, 当前子线程发起, 当前子线程处理
     *       2). 添加新的 fd, 添加任务节点的操作是由主线程发起的
     *   2. 不能让主线程处理任务队列, 需要由当前的子线程去处理
     */
    if (evLoop->threadID == pthread_self()) // 对于主线程来说，这一定成立
    {
        // 当前子线程处理任务队列，若是主线程，就是针对主线程的
        eventLoopProcessTask(evLoop);
    }
    else
    {
        // 主线程通知子线程处理任务队列
        taskWakeup(evLoop);
    }
    return 0;
}

// 处理任务队列中的任务
int eventLoopProcessTask(struct EventLoop *evLoop)
{
    pthread_mutex_lock(&evLoop->mutex);         // 加锁
    struct ChannelElement *head = evLoop->head; // 获取任务队列的头节点
    // 遍历任务队列
    while (head != NULL)
    {
        struct Channel *channel = head->channel;
        if (head->type == ADD)
        {
            // 添加事件
            eventLoopAdd(evLoop, channel);
        }
        else if (head->type == DELETE)
        {
            // 删除事件
            eventLoopRemove(evLoop, channel);
        }
        else if (head->type == MODIFY)
        {
            // 修改事件
            eventLoopModify(evLoop, channel);
        }
        struct ChannelElement *tmp = head;
        head = head->next;
        free(tmp); // 释放处理过的任务节点
    }
    evLoop->head = evLoop->tail = NULL;   // 清空任务队列
    pthread_mutex_unlock(&evLoop->mutex); // 解锁
    return 0;
}

// 添加事件
int eventLoopAdd(struct EventLoop *evLoop, struct Channel *channel)
{
    int fd = channel->fd;
    struct ChannelMap *channelMap = evLoop->channelMap;
    if (fd >= channelMap->size)
    {
        // 如果没有足够的空间存储键值对 fd - channel，则扩容
        if (!makeMapRoom(channelMap, fd, sizeof(struct Channel *)))
        {
            return -1;
        }
    }
    // 存储 fd 对应的 channel
    if (channelMap->list[fd] == NULL)
    {
        channelMap->list[fd] = channel;
        evLoop->dispatcher->add(channel, evLoop);
    }
    return 0;
}

// 删除事件
int eventLoopRemove(struct EventLoop *evLoop, struct Channel *channel)
{
    int fd = channel->fd;
    struct ChannelMap *channelMap = evLoop->channelMap;
    if (fd >= channelMap->size)
    {
        return -1;
    }
    int ret = evLoop->dispatcher->remove(channel, evLoop); // 从 dispatcher 中删除事件
    return ret;
}

// 修改事件
int eventLoopModify(struct EventLoop *evLoop, struct Channel *channel)
{
    int fd = channel->fd;
    struct ChannelMap *channelMap = evLoop->channelMap;
    if (channelMap->list[fd] == NULL)
    {
        return -1;
    }
    int ret = evLoop->dispatcher->modify(channel, evLoop); // 修改 dispatcher 中的事件
    return ret;
}

// 销毁 channel
int destroyChannel(struct EventLoop *evLoop, struct Channel *channel)
{
    // 删除 channel 和 fd 的对应关系
    evLoop->channelMap->list[channel->fd] = NULL;
    // 关闭 fd
    close(channel->fd);
    // 释放 channel
    free(channel);
    return 0;
}
