#include "ThreadPool.h"
#include <unistd.h>
 
// 构造函数
// threadNum：线程池里线程数量
// queSize：任务队列容量
// 初始化成员变量：线程数量、线程容器、队列大小、任务队列、退出标志
ThreadPool::ThreadPool(size_t threadNum, size_t queSize)
    : m_threadNum(threadNum) // 设置线程数量
      ,
      m_threads() // 用于存储线程对象的 vector，初始为空
      ,
      m_queSize(queSize) // 任务队列容量
      ,
      m_que(m_queSize) // 初始化任务队列对象
      ,
      m_isExit(false) // 线程池退出标志，默认 false
{
}

// 析构函数
// 自动释放线程容器和队列资源
ThreadPool::~ThreadPool()
{
}

// 启动线程池
void ThreadPool::start()
{
    for (size_t idx = 0; idx < m_threadNum; ++idx)
    {
        // 创建线程池线程
        // thread 对象不能被拷贝（拷贝构造被删除），只能移动或直接构造
        // 每个线程都执行 ThreadPool::doTask 入口函数，并绑定 this 指针
        m_threads.push_back(thread(&ThreadPool::doTask, this));
    }
}

// 停止线程池
void ThreadPool::stop()
{
    // 先等待任务队列清空，保证已经提交的任务都被执行完
    while (!m_que.empty())
    {
        sleep(1); // 每秒轮询一次队列状态
    }

    // 设置退出标志，告知工作线程可以安全退出
    m_isExit = true;

    // 唤醒可能阻塞在任务队列上的线程（TaskQueue::pop 可能阻塞）
    m_que.wakeup(); // 通知所有阻塞线程可以继续执行，退出循环

    // 等待所有子线程退出，避免主线程提前结束
    for (auto &th : m_threads)
    {
        th.join();
    }
}

// 向线程池添加任务
// rvalue 引用，支持移动语义，避免拷贝开销
void ThreadPool::addTask(ElemType &&ptask)
{
    // 安全检查，避免空指针调用任务导致段错误
    if (ptask)
    {
        // push 到任务队列
        // 如果队列满，会阻塞生产者线程
        // 如果队列未满，会添加任务并唤醒等待的消费者线程
        m_que.push(std::move(ptask));
    }
}

// 从任务队列获取任务
ElemType ThreadPool::getTask()
{
    // pop 已经实现了阻塞等待逻辑
    return m_que.pop();
}

// 工作线程入口函数
// 每个线程启动后都会调用 doTask
void ThreadPool::doTask()
{
    // 只要线程池未退出，就循环获取任务
    while (!m_isExit)
    {
        // 阻塞等待任务，如果队列为空线程会在 pop 里阻塞
        ElemType ptask = getTask();

        if (ptask)
        {
            // 执行任务
            ptask();
        }
        else
        {
            // 理论上不应该出现 nullptr
            cout << "ptask == nullptr" << endl;
        }
    }
}
