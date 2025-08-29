#ifndef _ECHOSERVER_H
#define _ECHOSERVER_H

#include "TcpServer.h"
#include "ThreadPool.h"

class EchoServer
{
public:
    EchoServer(size_t threadNum, size_t queSize, const string &ip, unsigned short port);

    ~EchoServer();

    void start();

    void stop();

    void onNewConnection(const TcpConnectionPtr &con);

    void onMessage(const TcpConnectionPtr &con);

    void onClose(const TcpConnectionPtr &con);

private:
    ThreadPool _pool;
    TcpServer _server;
};

#endif
