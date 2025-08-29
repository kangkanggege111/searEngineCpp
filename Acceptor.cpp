#include "Acceptor.h"
#include <iostream>
using std::cout;
using std::endl;
Acceptor::Acceptor(const string &ip, unsigned short port)
    : _sock(), _addr(ip, port)
{
}

Acceptor::~Acceptor()
{
}
void Acceptor::ready()
{
    setReuseAddr();
    setReusePort();
    bind();
    listen();
}
void Acceptor::setReuseAddr()
{
    int opt = 1;
    int retval = setsockopt(_sock.getFd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (retval == -1)
    {
        perror("setsockopt addr");
        return;
    }
}

void Acceptor::setReusePort()
{
    int opt = 1;
    int retval = setsockopt(_sock.getFd(), SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    if (retval == -1)
    {
        perror("setsockopt port");
        return;
    }
}

void Acceptor::bind()
{
    // 告诉编译器，这里调用的bind是C语言的库函数
    int ret = ::bind(_sock.getFd(),
                     (struct sockaddr *)_addr.getInetAddressPtr(),
                     sizeof(struct sockaddr)); // 将ip端口与文件描述符绑定
    if (ret == -1)
    {
        perror("bind");
        return;
    }
}

void Acceptor::listen()
{
    // 调用系统调用  控制同时等待连接的队列大小为128
    int ret = ::listen(_sock.getFd(), 128);
    if (ret == -1)
    {
        perror("listen");
        return;
    }

    cout << "listen ..." << endl;
}

int Acceptor::accept()
{
    int connfd = ::accept(_sock.getFd(), nullptr, nullptr);
    if (connfd == -1)
    {
        perror("accept");
        return -1;
    }
    return connfd;
}

int Acceptor::fd()
{
    return _sock.getFd();
}
