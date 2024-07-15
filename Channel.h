#pragma once // 防止头文件被重复包含

#include <stdbool.h> // 包含标准布尔类型的头文件

// 定义函数指针类型 handleFunc，指向返回值为 int，参数为 void* 的函数
typedef int (*handleFunc)(void *arg);

// 定义文件描述符的读写事件的枚举类型
enum FDEvent
{
    TimeOut = 0x01,   // 超时事件，值为 0x01
    ReadEvent = 0x02, // 读事件，值为 0x02
    WriteEvent = 0x04 // 写事件，值为 0x04
};

// 定义 Channel 结构体，用于管理文件描述符及其相关事件和回调函数
struct Channel
{
    int fd;                     // 文件描述符
    int events;                 // 事件掩码，用于标记文件描述符的事件类型
    handleFunc readCallback;    // 读事件的回调函数
    handleFunc writeCallback;   // 写事件的回调函数
    handleFunc destroyCallback; // 销毁事件的回调函数
    void *arg;                  // 回调函数的参数
};

// 初始化一个 Channel 结构体
struct Channel *channelInit(int fd, int events, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyFunc, void *arg);
// 参数：
// - fd: 文件描述符
// - events: 事件掩码
// - readFunc: 读事件的回调函数
// - writeFunc: 写事件的回调函数
// - destroyFunc: 销毁事件的回调函数
// - arg: 回调函数的参数

// 修改文件描述符fd的写事件（开启或关闭写事件检测）
void writeEventEnable(struct Channel *channel, bool flag);
// 参数：
// - channel: 指向 Channel 结构体的指针
// - flag: 布尔值，true 表示开启写事件检测，false 表示关闭写事件检测

// 判断是否需要检测文件描述符的写事件
bool isWriteEventEnable(struct Channel *channel);
// 参数：
// - channel: 指向 Channel 结构体的指针
// 返回值：
// - 如果需要检测写事件，返回 true；否则返回 false