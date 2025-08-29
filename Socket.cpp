#include "Socket.h"

Socket::Socket()
{
    // 创建一个套接字
    // 协议族:AF_INET 表示IPv4
    // 套接字类型: SOCK_STREAM 字节流套接字
    _fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_fd == -1)
    {
        perror("socket");
        return;
    }
}

Socket::Socket(int fd)
    : _fd(fd)
{
}

Socket::~Socket()
{
    close(_fd);
}

int Socket::getFd()
{
    return _fd;
}
