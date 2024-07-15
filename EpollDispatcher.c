#pragma once // 防止头文件被重复包含

#include "Dispatcher.h" // 包含 Dispatcher.h 头文件
#include <sys/epoll.h>  // 包含 epoll 系统调用的头文件
#include <stdlib.h>     // 包含标准库函数，如 malloc, free
#include <unistd.h>     // 包含 POSIX 操作系统 API，如 close
#include <stdio.h>      // 标准输入输出库，用于 perror, printf 等函数

#define Max 520 // 定义常量 Max，表示事件数组的最大大小

// 定义 EpollData 结构体，保存 epoll 相关数据
struct EpollData
{
    int epfd;                   // epoll 文件描述符
    struct epoll_event *events; // 事件数组
};

// 函数声明
static void *epollInit();                                                       // 初始化 epoll
static int epollAdd(struct Channel *channel, struct EventLoop *evLoop);         // 添加事件
static int epollRemove(struct Channel *channel, struct EventLoop *evLoop);      // 删除事件
static int epollModify(struct Channel *channel, struct EventLoop *evLoop);      // 修改事件
static int epollDispatch(struct EventLoop *evLoop, int timeout);                // 事件分发
static int epollClear(struct EventLoop *evLoop);                                // 清理 epoll
static int epollCtl(struct Channel *channel, struct EventLoop *evLoop, int op); // 控制 epoll 操作

// 定义全局的 EpollDispatcher 结构体，并初始化其成员函数
struct Dispatcher EpollDispatcher = {
    epollInit,
    epollAdd,
    epollRemove,
    epollModify,
    epollDispatch,
    epollClear};

// 初始化 epoll
static void *epollInit()
{
    // 分配 EpollData 结构体的内存
    struct EpollData *data = (struct EpollData *)malloc(sizeof(struct EpollData));
    // 创建 epoll 实例
    data->epfd = epoll_create(10);
    if (data->epfd == -1)
    {
        perror("epoll_create"); // 输出错误信息
        exit(0);                // 退出程序
    }
    // 为事件数组分配内存并初始化
    data->events = (struct epoll_event *)calloc(Max, sizeof(struct epoll_event));
    return data; // 返回 epoll 数据
}

// 控制 epoll 操作
static int epollCtl(struct Channel *channel, struct EventLoop *evLoop, int op)
{
    // 获取 epoll 数据
    struct EpollData *data = (struct EpollData *)evLoop->dispatcherData;
    struct epoll_event ev;
    ev.data.fd = channel->fd; // 设置文件描述符
    int events = 0;
    if (channel->events & ReadEvent) // 检查是否有读事件
    {
        events |= EPOLLIN; // 设置读事件
    }
    if (channel->events & WriteEvent) // 检查是否有写事件
    {
        events |= EPOLLOUT; // 设置写事件
    }
    ev.events = events; // 设置事件
    // 调用 epoll_ctl 函数
    int ret = epoll_ctl(data->epfd, op, channel->fd, &ev);
    return ret; // 返回结果
}

// 添加事件
static int epollAdd(struct Channel *channel, struct EventLoop *evLoop)
{
    // 调用 epollCtl 函数，执行 EPOLL_CTL_ADD 操作
    int ret = epollCtl(channel, evLoop, EPOLL_CTL_ADD);
    if (ret == -1)
    {
        perror("epoll_crl add"); // 输出错误信息
        exit(0);                 // 退出程序
    }
    return ret; // 返回结果
}

// 删除事件
static int epollRemove(struct Channel *channel, struct EventLoop *evLoop)
{
    // 调用 epollCtl 函数，执行 EPOLL_CTL_DEL 操作
    int ret = epollCtl(channel, evLoop, EPOLL_CTL_DEL);
    if (ret == -1)
    {
        perror("epoll_crl delete"); // 输出错误信息
        exit(0);                    // 退出程序
    }
    // 通过 channel 释放对应的 TcpConnection 资源
    channel->destroyCallback(channel->arg);
    return ret; // 返回结果
}

// 修改事件
static int epollModify(struct Channel *channel, struct EventLoop *evLoop)
{
    // 调用 epollCtl 函数，执行 EPOLL_CTL_MOD 操作
    int ret = epollCtl(channel, evLoop, EPOLL_CTL_MOD);
    if (ret == -1)
    {
        perror("epoll_crl modify"); // 输出错误信息
        exit(0);                    // 退出程序
    }
    return ret; // 返回结果
}

// 事件分发
static int epollDispatch(struct EventLoop *evLoop, int timeout)
{
    // 获取 epoll 数据
    struct EpollData *data = (struct EpollData *)evLoop->dispatcherData;
    // 调用 epoll_wait 函数，等待事件发生
    int count = epoll_wait(data->epfd, data->events, Max, timeout * 1000);
    for (int i = 0; i < count; ++i)
    {
        int events = data->events[i].events; // 获取事件
        int fd = data->events[i].data.fd;    // 获取文件描述符
        if (events & EPOLLERR || events & EPOLLHUP)
        {
            // 对方断开了连接，继续处理其他事件
            continue;
        }
        if (events & EPOLLIN) // 处理读事件
        {
            eventActivate(evLoop, fd, ReadEvent);
        }
        if (events & EPOLLOUT) // 处理写事件
        {
            eventActivate(evLoop, fd, WriteEvent);
        }
    }
    return 0; // 返回结果
}

// 清理 epoll
static int epollClear(struct EventLoop *evLoop)
{
    // 获取 epoll 数据
    struct EpollData *data = (struct EpollData *)evLoop->dispatcherData;
    free(data->events); // 释放事件数组的内存
    close(data->epfd);  // 关闭 epoll 文件描述符
    free(data);         // 释放 EpollData 结构体的内存
    return 0;           // 返回结果
}
