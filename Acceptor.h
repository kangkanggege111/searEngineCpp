#ifndef _ACCEPTOR_H
#define _ACCEPTOR_H

#include "Socket.h"
#include "InetAddress.h"

class Acceptor
{
public:
    // 有参构造
    Acceptor(const string &ip, unsigned short port);

    // 析构函数
    ~Acceptor();

    void ready();

    // 接受一个客户端连接，返回新的套接字描述符 connfd
    // 原来的 _sock 保持监听(listen)状态，可以继续接受其他连接
    int accept();

    int fd();

private:
    // Socket有默认构造,会创建一个文件描述符
    Socket _sock;
    InetAddress _addr;

    // 设置ip地址复用
    void setReuseAddr();

    // 设置端口复用
    void setReusePort();

    void bind();

    // 调用系统调用让套接字进入被动监听状态,准备接收客户端的连接
    void listen();
};

#endif
