#pragma once // 防止头文件被重复包含

#include <stdbool.h>    // 包含标准布尔类型头文件
#include "Dispatcher.h" // 包含 Dispatcher 头文件
#include "ChannelMap.h" // 包含 ChannelMap 头文件
#include <pthread.h>    // 包含 POSIX 线程库头文件

// 声明外部的 Dispatcher 变量
extern struct Dispatcher EpollDispatcher;
extern struct Dispatcher PollDispatcher;
extern struct Dispatcher SelectDispatcher;

// 定义处理节点中的 channel 的方式的枚举类型
enum ElemType
{
    ADD,    // 添加
    DELETE, // 删除
    MODIFY  // 修改
};

// 定义任务队列的节点结构体
struct ChannelElement
{
    int type;                    // 如何处理该节点中的 channel
    struct Channel *channel;     // 指向 channel 结构体的指针
    struct ChannelElement *next; // 指向下一个 ChannelElement 节点的指针
};

struct Dispatcher; // 前向声明 Dispatcher 结构体

// 定义事件循环结构体
struct EventLoop
{
    bool isQuit;                   // 标志事件循环是否退出
    struct Dispatcher *dispatcher; // 指向 Dispatcher 结构体的指针，epoll,poll,select
    void *dispatcherData;          // 指向 dispatcher 相关数据的指针
    // 任务队列
    struct ChannelElement *head; // 任务队列的头指针
    struct ChannelElement *tail; // 任务队列的尾指针
    // map,ChannelMap
    struct ChannelMap *channelMap; // 指向 ChannelMap 结构体的指针
    // 线程 id, name, mutex
    pthread_t threadID;    // 线程ID
    char threadName[32];   // 线程名称
    pthread_mutex_t mutex; // 互斥锁,用来保护任务队列的
    int socketPair[2];     // 存储本地通信的文件描述符，通过 socketpair 初始化
};

// 初始化事件循环
struct EventLoop *eventLoopInit();                         // 初始化事件循环
struct EventLoop *eventLoopInitEx(const char *threadName); // 带线程名的初始化，主要是主线程和子线程区分

// 启动反应堆模型
int eventLoopRun(struct EventLoop *evLoop); // 启动事件循环

// 处理被激活的文件描述符
int eventActivate(struct EventLoop *evLoop, int fd, int event); // 激活事件

// 添加任务到任务队列
int eventLoopAddTask(struct EventLoop *evLoop, struct Channel *channel, int type); // 添加任务

// 处理任务队列中的任务
int eventLoopProcessTask(struct EventLoop *evLoop); // 处理任务队列

// 处理 dispatcher 中的节点
int eventLoopAdd(struct EventLoop *evLoop, struct Channel *channel);    // 添加事件
int eventLoopRemove(struct EventLoop *evLoop, struct Channel *channel); // 删除事件
int eventLoopModify(struct EventLoop *evLoop, struct Channel *channel); // 修改事件

// 释放 channel
int destroyChannel(struct EventLoop *evLoop, struct Channel *channel); // 销毁 channel
