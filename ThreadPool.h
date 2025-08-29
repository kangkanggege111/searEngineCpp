#ifndef _THREADPOOL_H
#define _THREADPOOL_H
#include "TaskQueue.h"
#include <vector>
using std::vector;

class ThreadPool
{
public:
    // 构造函数的两个参数代表子线程的个数和任务队列的容量
    ThreadPool(size_t threadNum, size_t queSize);
    ~ThreadPool();

    // 提供线程池启动和关闭的函数
    void start();
    void stop();

    // 生产者线程可以往线程池中添加任务
    void addTask(ElemType &&ptask);

private:
    size_t m_threadNum;
    vector<thread> m_threads;
    size_t m_queSize;
    TaskQueue m_que;
    bool m_isExit;

    // 在doTask中调用，就是从任务队列中取任务以便后续执行
    ElemType getTask();

    // 注册给工作线程作为入口函数
    void doTask();
};

#endif
