#ifndef _EVENTLOOP_H
#define _EVENTLOOP_H

#include "Acceptor.h"
#include "TcpConnection.h"
#include <sys/epoll.h>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <mutex>
using std::bind;
using std::function;
using std::lock_guard;
using std::map;
using std::mutex;
using std::shared_ptr;
using std::vector;

using TcpConnectionPtr = shared_ptr<TcpConnection>;
using TcpConnectionCallback = function<void(const TcpConnectionPtr &)>;
using Functor = function<void()>;

class EventLoop
{
public:
    EventLoop(Acceptor &acc);

    ~EventLoop();

    void loop();

    void unloop();

    void setNewConnectionCallback(TcpConnectionCallback &&cb);
    void setMessageCallback(TcpConnectionCallback &&cb);
    void setCloseCallback(TcpConnectionCallback &&cb);

    // eventfd代表计数器
    // 创建文件描述符，以及对文件描述符进行读写
    int createEventfd();
    void handleRead();
    void wakeup();

    void runInLoop(Functor &&cb);
    void doPendingFunctors();

private:
    int createEpollFd();

    void addEpollReadFd(int fd);

    void delEpollReadFd(int fd);

    void waitEpollFd();

    void handleNewConnection();

    void handleMessage(int fd);

private:
    int _epfd;
    vector<struct epoll_event> _evlist; // epoll_event的就绪列表
    bool _isLooping;                    // 事件开关循环
    Acceptor &_acceptor;
    map<int, shared_ptr<TcpConnection>> _conns;

    TcpConnectionCallback _onNewConnection;
    TcpConnectionCallback _onMessage;
    TcpConnectionCallback _onClose;

    int _evtfd;
    vector<Functor> _pendings;
    mutex _mutex;
};

#endif
