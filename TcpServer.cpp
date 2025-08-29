#include "TcpServer.h"

// TcpServer 构造函数
// 参数: ip 和 port 用来初始化 Acceptor（负责监听）
// 同时传入 Acceptor 给 EventLoop，保证 Loop 里能知道监听 socket 的 fd
TcpServer::TcpServer(const string &ip, unsigned short port)
    : _acceptor(ip, port) // 初始化监听器（封装 socket、bind、listen）
      ,
      _loop(_acceptor) // 初始化事件循环（reactor），并绑定监听器
{
}

// TcpServer 析构函数
// 一般做资源回收，这里是空实现，交给成员自己析构
TcpServer::~TcpServer()
{
}

// 设置所有回调函数
// cb1：新连接到来时的回调
// cb2：消息到来时的回调
// cb3：连接关闭时的回调
void TcpServer::setAllCallback(callback &&cb1, callback &&cb2, callback &&cb3)
{
    // 转发到 EventLoop 里去设置相应的回调
    // std::move 表示“右值引用转移”，避免不必要的拷贝
    _loop.setNewConnectionCallback(std::move(cb1));
    _loop.setMessageCallback(std::move(cb2));
    _loop.setCloseCallback(std::move(cb3));
}

// 启动服务器
void TcpServer::start()
{
    // 1. 让 Acceptor 准备好监听 (bind + listen)
    _acceptor.ready();

    // 2. 进入事件循环，开始处理事件（accept、读写、关闭等）
    _loop.loop();
}

// 停止服务器
void TcpServer::stop()
{
    // 修改 loop 内部的标志位，安全退出 epoll 循环
    _loop.unloop();
}
