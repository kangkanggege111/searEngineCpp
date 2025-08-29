#include "EventLoop.h"   // 事件循环类的声明
#include <sys/eventfd.h> // eventfd(2)：用户空间轻量通知机制
#include <iostream>      // 标准输出/错误
using std::cerr;
using std::cout;
using std::endl;

EventLoop::EventLoop(Acceptor &acc)
    : _epfd(createEpollFd()),  // 创建 epoll 实例，返回 epoll fd
      _evlist(1024),           // 预分配 1024 个 epoll_event 的就绪列表
      _isLooping(false),       // 事件循环开关，初始未运行
      _acceptor(acc),          // 引用传入的 Acceptor，用于获取 listenfd / accept()
      _conns(),                // 连接表：fd -> TcpConnectionPtr
      _evtfd(createEventfd()), // eventfd：跨线程唤醒 epoll_wait
      _pendings(),             // 待执行任务队列（跨线程投递）
      _mutex()                 // 保护 _pendings 的互斥量
{
    // 因为数据成员vector是在epoll_wait函数中充当就绪列表
    // 所以一定预留存放元素的空间
    // 如果是一个空的vector，那么也至少应该reverse
    // ↑ 这里注释里“reverse”应为“reserve”，不过你已经用构造函数分配了 1024 个元素 ✅

    // 需要将listenfd加到红黑树上进行监听
    // 要listenfd，需要Acceptor中添加一个函数来获取
    int listenfd = _acceptor.fd(); // 从 Acceptor 获取监听套接字 fd
    addEpollReadFd(listenfd);      // 监听“有新连接到来”的可读事件

    // 也要监听用于通知的文件描述符
    addEpollReadFd(_evtfd); // 监听 eventfd（用于跨线程唤醒）
}

EventLoop::~EventLoop()
{
    close(_epfd); // 释放 epoll 实例（注意：evtfd/connfd/lsnfd通常也需要在别处清理）
}

// 让进程不停地监听事件
void EventLoop::loop()
{
    _isLooping = true; // 打开循环开关
    while (_isLooping) // 主循环：阻塞等待/分发事件
    {
        cout << "等待客户端........" << endl;
        waitEpollFd(); // 核心：epoll_wait + 分派
    }
}

// 通过更改标志(成员变量_isLooping的方式)来关闭监听事件的死循环
void EventLoop::unloop()
{
    _isLooping = false; // 关闭循环；通常配合 wakeup() 让 epoll_wait 立即返回
}

// 创建epoolfd
int EventLoop::createEpollFd()
{
    int fd = epoll_create(100); // 创建 epoll（参数仅需 >0；现代推荐 epoll_create1）
    if (fd == -1)
    {
        perror("createEpollFd"); // 失败打印 errno
        return -1;
    }
    return fd; // 返回 epoll fd
}

// 添加文件描述符进监听集合
void EventLoop::addEpollReadFd(int fd)
{
    struct epoll_event evt;
    evt.events = EPOLLIN; // 关心“可读”事件（水平触发）
    evt.data.fd = fd;     // 回传的数据携带 fd 本身（简洁）

    int ret = epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &evt); // 将 fd 加入 epoll 关注集合
    if (-1 == ret)
    {
        perror("addEpollReadFd"); // 失败处理
        return;
    }
}

// 从监听集合中移除该文件描述符
void EventLoop::delEpollReadFd(int fd)
{
    struct epoll_event evt;
    evt.events = EPOLLIN; // 与添加时一致（DEL 时内核通常忽略该字段）
    evt.data.fd = fd;

    int ret = epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, &evt); // 从 epoll 关注集合删除
    if (-1 == ret)
    {
        perror("delEpollReadFd");
        return;
    }
}

// 等待监听集合中的文件描述符的响应
void EventLoop::waitEpollFd()
{
    int nready = 0;
    do
    {
        // vector充当就绪列表，
        // 其实是需要拿存放元素的空间来存放epoll_event
        // 这里要考虑如何获取vector存放元素的空间的首地址
        nready = epoll_wait(_epfd, _evlist.data(), _evlist.size(), 10000);
        // ↑ 等待事件到来，超时 3000ms；把就绪的 epoll_event 写到 _evlist 的连续内存里
    } while (-1 == nready && errno == EINTR); // 被信号中断则重试（健壮性）

    if (-1 == nready)
    {
        cerr << "-1 == nready" << endl; // 真正的错误（非 EINTR）
        return;
    }
    else if (0 == nready)
    {
        // 超时
        cout << ">> epoll_wait timeout!!!" << endl; // 超时无事件（可用于定时任务 tick）
    }
    else
    {
        // 如果nready已经到达vector的容量上限
        // 那么后续就绪的文件描述很可能就会超过vector的容量
        // 那么需要进行扩容
        // 但是自动扩容的途径如push_back/insert等
        // 不可能在epoll_wait中进行调用，
        // 所以只能手动扩容
        if (nready == (int)_evlist.size())
        {
            // 如果采用push_back，确实可以扩容
            // 但是扩容之后，size实际上等于1025
            // 而epoll_wait没有改变vector的size的能力
            cout << "_evlist监听集合慢,开始扩容...." << endl;
            _evlist.resize(2 * nready); // 就绪条目“打满”时，手动把容量翻倍，避免下次溢出
        }

        for (int idx = 0; idx < nready; ++idx) // 遍历所有就绪事件
        {
            int listenfd = _acceptor.fd(); // 获取监听 fd（可缓存以减少系统调用/函数调用）
            int fd = _evlist[idx].data.fd; // 取出触发事件的 fd
            cout << "接收到了触发事件的fd.." << fd << endl;
            if (fd == listenfd) // 有新的连接
            {
                cout << " EventLoop 开始处理新链接" << endl;
                handleNewConnection(); // 处理新连接：accept + 注册 + 回调
            }
            else if (fd == _evtfd)
            {
                // 针对eventfd进行读操作，
                // 也就是将计数器清空
                cout << "针对eventfd进行读操作  将计数器清空" << endl;
                handleRead(); // 把 eventfd 的计数器读掉（清零），避免反复触发

                // 收到通知之后，执行任务
                doPendingFunctors(); // 把跨线程投递的任务取出执行
            }
            else
            {
                cout << " EventLoop 老的连接有数据传入" << endl;
                // 老的连接有数据传入
                handleMessage(fd); // 处理已建立连接上的读事件（或关闭）
            }
        }
    }
}

// 处理新的连接
void EventLoop::handleNewConnection()
{
    // 只要accept函数执行成功，表明三次握手建立成功
    // 从全连接队列中取出一条新的连接
    int connfd = _acceptor.accept(); // accept 返回“连接套接字”
    if (-1 == connfd)
    {
        perror("handleNewConnection"); // 失败（常见：EMFILE/ENFILE/资源不足/非阻塞下EAGAIN）
        return;
    }

    // connfd放到红黑树上进行监听
    addEpollReadFd(connfd); // 关注该连接上的可读事件

    // 利用代表连接的文件描述符connfd创建堆上的TcpConnection
    // 交给shared_ptr管理
    TcpConnectionPtr con(new TcpConnection(connfd, this)); // 封装连接对象，交给智能指针

    // 创建完TcpConnection之后，它内部的function是空的
    // 所以需要由EventLoop再做一次转交，让TcpConnection的function
    // 也能关联外部的函数
    con->setNewConnectionCallback(_onNewConnection); // 设置三类回调
    con->setMessageCallback(_onMessage);
    con->setCloseCallback(_onClose);

    /* _conns.insert({connfd,con}); */
    _conns[connfd] = con; // 放入连接表，便于后续通过 fd 查找

    // 新连接建立的回调执行
    con->handleNewConnectionCallback(); // 触发“新连接到来”的业务回调
}

// 处理旧链接(处理旧链接的请求(信息))
void EventLoop::handleMessage(int fd)
{
    auto it = _conns.find(fd); // 通过 fd 找到对应的连接对象
    if (it != _conns.end())
    {
        // 先判断连接是否已经断开了
        bool flag = it->second->isClosed(); // 探测对端是否关闭（常见实现：peek/recv返回0）

        if (flag)
        {
            // 连接断开情况的回调
            it->second->handleCloseCallback(); // 业务层“连接关闭”回调

            // 如果连接断开了，那么就要
            // 从红黑树上删除fd，取消监听
            // 以及从map中删除对应的记录
            delEpollReadFd(fd); // epoll 取消关注
            _conns.erase(it);   // 从连接表移除
        }
        else
        {
            cout << "Eventloop中 调用handleMessageCallback回调函数" << endl;
            it->second->handleMessageCallback();
        }
    }
    else
    {
        cout << "连接不存在" << endl; // 防御：fd 不在连接表中
        return;
    }
}

void EventLoop::setNewConnectionCallback(TcpConnectionCallback &&cb)
{
    _onNewConnection = std::move(cb); // 设置“新连接”回调
}
void EventLoop::setMessageCallback(TcpConnectionCallback &&cb)
{
    _onMessage = std::move(cb); // 设置“消息到达”回调
}
void EventLoop::setCloseCallback(TcpConnectionCallback &&cb)
{
    _onClose = std::move(cb); // 设置“连接关闭”回调
}

// 创建一个_eventfd 用于线程之间的通知
int EventLoop::createEventfd()
{
    int fd = ::eventfd(0, 0); // 创建 eventfd，初始计数 0（建议：可加 EFD_NONBLOCK|EFD_CLOEXEC）
    if (fd == -1)
    {
        perror("createEventfd");
        return -1;
    }
    return fd; // 返回 eventfd
}

// 读取eventfd,将计数器清空
void EventLoop::handleRead()
{
    uint64_t val = 1;                                   // 预设值无所谓（read 会覆盖），这里设 1 也行
    ssize_t ret = read(_evtfd, &val, sizeof(uint64_t)); // 读取并清空 eventfd 计数器（返回读到的累积值）
    if (ret != sizeof(uint64_t))
    {
        perror("handleRead"); // 仅读到部分数据（理论上 eventfd 要么 8 字节，要么阻塞/出错）
        return;
    }
}
void EventLoop::wakeup()
{
    uint64_t val = 1;                                    // 向 eventfd 写入 1：计数器 += 1
    ssize_t ret = write(_evtfd, &val, sizeof(uint64_t)); // 唤醒正在 epoll_wait 的线程
    if (ret != sizeof(uint64_t))
    {
        perror("wakeup"); // 写入失败：常见为非阻塞且计数满（很少见）
        return;
    }
}

void EventLoop::runInLoop(Functor &&cb)
{
    {
        lock_guard<mutex> lg(_mutex);       // 加锁保护任务队列
        _pendings.push_back(std::move(cb)); // 跨线程投递任务到主循环执行
    }

    // 向计数器中写入数值
    // 主线程监听着eventfd，发现计数器中的值不为0，
    // 即代表eventfd就绪，相应地去执行任务
    wakeup(); // 用 eventfd 唤醒 epoll_wait，使其尽快执行任务
}

void EventLoop::doPendingFunctors()
{
    // 针对EventLoop中的任务队列
    // 就类似于生产者消费者模型中的数据仓库
    // 生产者（子线程）向仓库中添加任务，是持有锁的
    // 消费者（主线程）从仓库中取走任务，也是持有锁的
    vector<Functor> temp;
    {
        lock_guard<mutex> lg(_mutex); // 取任务也需要加锁
        temp.swap(_pendings);         // 一次性“搬空”队列，降低锁持有时间
    }

    // 主线程自己慢慢执行
    for (auto &cb : temp)
    {
        cb(); // 逐个执行投递的任务（避免在持锁状态下执行用户回调）
    }
}
