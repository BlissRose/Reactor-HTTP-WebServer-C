#include "ThreadPool.h" // 包含 ThreadPool 头文件
#include <assert.h>     // 包含断言头文件，用于条件检查
#include <stdlib.h>     // 包含标准库函数，如 malloc, free

// 初始化线程池
struct ThreadPool *threadPoolInit(struct EventLoop *mainLoop, int count)
{
    // 分配 ThreadPool 结构体的内存
    struct ThreadPool *pool = (struct ThreadPool *)malloc(sizeof(struct ThreadPool));
    pool->index = 0;           // 初始化索引为 0
    pool->isStart = false;     // 初始化线程池启动标志为 false
    pool->mainLoop = mainLoop; // 设置主事件循环
    pool->threadNum = count;   // 设置线程数量
    // 分配 WorkerThread 数组的内存
    pool->workerThreads = (struct WorkerThread *)malloc(sizeof(struct WorkerThread) * count);
    return pool; // 返回线程池指针
}

// 启动线程池，必须是主线程启动线程池
void threadPoolRun(struct ThreadPool *pool)
{
    assert(pool && !pool->isStart); // 确保线程池存在且未启动
    if (pool->mainLoop->threadID != pthread_self())
    {
        exit(0); // 如果当前线程不是主事件循环的线程，则退出
    }
    pool->isStart = true; // 设置线程池启动标志为 true
    if (pool->threadNum)
    {
        // 初始化并启动每个工作线程
        for (int i = 0; i < pool->threadNum; ++i)
        {
            workerThreadInit(&pool->workerThreads[i], i); // 初始化工作线程
            workerThreadRun(&pool->workerThreads[i]);     // 启动工作线程
        }
    }
}

// 获取工作线程的事件循环，// 取出线程池中的某个子线程的反应堆实例
struct EventLoop *takeWorkerEventLoop(struct ThreadPool *pool)
{
    assert(pool->isStart); // 确保线程池已启动
    if (pool->mainLoop->threadID != pthread_self())
    {
        exit(0); // 如果当前线程不是主事件循环的线程，则退出
    }
    // 从线程池中找一个子线程，并取出其事件循环实例
    struct EventLoop *evLoop = pool->mainLoop; // 默认使用主事件循环
    if (pool->threadNum > 0)
    {
        // 轮询选择一个工作线程的事件循环
        evLoop = pool->workerThreads[pool->index].evLoop;
        pool->index = ++pool->index % pool->threadNum; // 更新索引，轮询选择，雨露均沾
    }
    return evLoop; // 返回选中的事件循环实例
}
