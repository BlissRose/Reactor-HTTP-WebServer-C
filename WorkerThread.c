#include "WorkerThread.h" // 包含 WorkerThread 头文件
#include <stdio.h>        // 标准输入输出库，用于 printf, sprintf 等函数

// 初始化 WorkerThread 结构体
int workerThreadInit(struct WorkerThread *thread, int index)
{
    thread->evLoop = NULL;                        // 将事件循环指针初始化为 NULL
    thread->threadID = 0;                         // 将线程 ID 初始化为 0
    sprintf(thread->name, "SubThread-%d", index); // 设置线程名称，格式为 "SubThread-index"
    pthread_mutex_init(&thread->mutex, NULL);     // 初始化互斥锁
    pthread_cond_init(&thread->cond, NULL);       // 初始化条件变量
    return 0;                                     // 返回 0，表示成功
}

// 子线程的回调函数
void *subThreadRunning(void *arg)
{
    struct WorkerThread *thread = (struct WorkerThread *)arg; // 将参数转换为 WorkerThread 结构体指针
    pthread_mutex_lock(&thread->mutex);                       // 加锁，保护共享资源
    thread->evLoop = eventLoopInitEx(thread->name);           // 初始化事件循环
    pthread_mutex_unlock(&thread->mutex);                     // 解锁
    pthread_cond_signal(&thread->cond);                       // 发出条件变量信号，通知主线程事件循环已初始化
    eventLoopRun(thread->evLoop);                             // 运行事件循环
    return NULL;                                              // 线程函数返回 NULL
}

// 启动 WorkerThread
void workerThreadRun(struct WorkerThread *thread)
{
    // 创建子线程，执行 subThreadRunning 函数
    pthread_create(&thread->threadID, NULL, subThreadRunning, thread);
    // 阻塞主线程，等待子线程完成事件循环初始化
    pthread_mutex_lock(&thread->mutex); // 加锁，保护共享资源
    while (thread->evLoop == NULL)      // 检查事件循环是否已初始化
    {
        pthread_cond_wait(&thread->cond, &thread->mutex); // 等待条件变量信号
    }
    pthread_mutex_unlock(&thread->mutex); // 解锁
}
