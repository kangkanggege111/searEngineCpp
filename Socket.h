#ifndef _SOCKET_H
#define _SOCKET_H

#include "NonCopyable.h"
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>

class Socket : public NonCopyable
{
public:
    // 无参默认构造(创建一个协议族为IPv4的字节流套接字)
    Socket();

    // 有参构造
    // explicit: 防止编译器进行隐式转换
    explicit Socket(int fd);

    // 析构函数
    ~Socket();

    // 获取成员变量(流套接字)
    int getFd();

private:
    int _fd;
};

#endif
