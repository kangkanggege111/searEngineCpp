#ifndef _TCPCONNECTION_H
#define _TCPCONNECTION_H

#include "SocketIO.h"
#include "Socket.h"
#include "InetAddress.h"
#include <string>
#include <memory>
#include <functional>
using std::function;
using std::shared_ptr;
using std::string;

class TcpConnection;
class EventLoop;

using TcpConnectionPtr = shared_ptr<TcpConnection>;
using TcpConnectionCallback = function<void(const TcpConnectionPtr &)>;

class TcpConnection
    : public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(int fd, EventLoop *loop);

    ~TcpConnection();

    bool isClosed();

    Message receive();

    void send(const string &msg);

    void sendInLoop(const string &msg);

    // 打印连接的信息
    string toString();

    // 这三个set函数中不能使用移动语义，转接EventLoop的function
    // 所以形参建议不使用右值引用，而是使用const左值引用
    void setNewConnectionCallback(const TcpConnectionCallback &cb);
    void setMessageCallback(const TcpConnectionCallback &cb);
    void setCloseCallback(const TcpConnectionCallback &cb);

    void handleNewConnectionCallback();
    void handleMessageCallback();
    void handleCloseCallback();

private:
    // 获取本端与对端的ip地址和端口号
    InetAddress getLocalAddr();
    InetAddress getPeerAddr();

private:
    // 因为TcpConnection的sendInLoop中需要调用
    // EventLoop的runInLoop函数
    EventLoop *_loop;

    SocketIO _sockIO;

    // 为了打印信息加入的数据成员
    Socket _sock;
    InetAddress _localAddr;
    InetAddress _peerAddr;

    // 回调
    TcpConnectionCallback _onNewConnection;
    TcpConnectionCallback _onMessage;
    TcpConnectionCallback _onClose;
};

#endif
