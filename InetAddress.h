#ifndef _INETADDRESS_H
#define _INETADDRESS_H

#include <arpa/inet.h>
#include <string.h>
#include <string>
using std::string;

class InetAddress
{
public:
    // 有参构造
    InetAddress(const string &ip, unsigned short port);
    // 拷贝构造
    InetAddress(const struct sockaddr_in &addr);
    // 析构函数
    ~InetAddress();
    // 获取成员变量(结构体_addr)的ip字段
    string getIp();
    // 获取成员变量(结构体_addr)的Port字段
    unsigned short getPort();

    const struct sockaddr_in *getInetAddressPtr();

private:
    struct sockaddr_in _addr;
};

#endif
