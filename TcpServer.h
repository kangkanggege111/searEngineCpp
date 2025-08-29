#ifndef _TCPSERVER_H
#define _TCPSERVER_H

#include "Acceptor.h"
#include "EventLoop.h"

class TcpServer
{
public:
    using callback = function<void(const TcpConnectionPtr &)>;

    TcpServer(const string &ip, unsigned short port);

    ~TcpServer();

    void start();

    void stop();

    void setAllCallback(callback &&cb1, callback &&cb2, callback &&cb3);

private:
    Acceptor _acceptor;
    EventLoop _loop;
};

#endif
