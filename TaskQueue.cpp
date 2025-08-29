#include "TaskQueue.h"

// 构造函数
// capa: 队列容量
TaskQueue::TaskQueue(size_t capa)
    : m_capacity(capa) // 初始化队列容量
{
}

// 析构函数
TaskQueue::~TaskQueue()
{
}

// 生产者线程调用 push 函数，将任务加入队列
void TaskQueue::push(ElemType &&value)
{
    // 1. 上锁，保证对队列的操作互斥
    unique_lock<mutex> ul(m_mtx);

    // 2. 队列满了就阻塞生产者
    while (full()) // 用 while 防止虚假唤醒
    {
        // 阻塞生产者线程，底层会先释放锁
        // 等到被唤醒时再重新持有锁继续执行
        m_notFull.wait(ul);
    }

    // 3. 队列未满，将任务加入队列
    m_que.push(std::move(value));

    // 4. 唤醒一个阻塞在 m_notEmpty 条件变量上的消费者线程
    m_notEmpty.notify_one();

    // 5. 解锁自动进行 (unique_lock 作用域结束)
}

// 消费者线程调用 pop 函数，从队列取任务
ElemType TaskQueue::pop()
{
    // 1. 上锁，保证对队列的操作互斥
    unique_lock<mutex> ul(m_mtx);

    // 2. 队列为空且线程池未退出，就阻塞消费者线程
    while (empty() && m_flag)
    {
        // 阻塞消费者线程，底层释放锁，等待唤醒
        m_notEmpty.wait(ul);
    }

    // 3. 队列中有任务或线程池退出，才进行取任务
    if (m_flag) // true 表示线程池未退出
    {
        // 取出队列最前面的任务
        ElemType temp = m_que.front();
        m_que.pop();

        // 唤醒一个阻塞在 m_notFull 条件变量上的生产者线程
        m_notFull.notify_one();

        return temp;
    }
    else
    {
        // 线程池退出，返回空指针
        return nullptr;
    }
}

// 判断队列是否满
bool TaskQueue::full()
{
    return m_que.size() == m_capacity;
}

// 判断队列是否为空
bool TaskQueue::empty()
{
    return m_que.size() == 0;
}

// 唤醒所有阻塞的消费者线程（线程池退出时调用）
// 通过 m_flag 标志，告诉消费者线程不要再阻塞等待
void TaskQueue::wakeup()
{
    m_flag = false;          // 修改标志位，表示线程池要退出
    m_notEmpty.notify_all(); // 唤醒所有可能阻塞的消费者线程
}
