#ifndef _SOCKETIO_H
#define _SOCKETIO_H

#include <unistd.h>
#include <stdio.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <vector>
#include <iostream>
struct Message
{
    int tag;           // 消息的类型 1: 关键字推荐 2: 网页搜索
    int length;        // value的长度
    std::string value; // 消息的内容
};
class SocketIO
{
public:
    // 有参构造
    explicit SocketIO(int fd);

    // 析构
    ~SocketIO();

    int readMessage(Message &msg);
    // 可靠地从套接字读指定长度的数据
    int readn(char *buf, int len);

    // 可靠地从套接字中按行读取数据
    int readLine(char *buf, int len);

    int writen(const char *buf, int len);

private:
    int _fd;
};

#endif
