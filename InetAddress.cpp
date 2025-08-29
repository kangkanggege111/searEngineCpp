#include "InetAddress.h"

InetAddress::InetAddress(const string &ip, unsigned short port)
{
    memset(&_addr, 0, sizeof(_addr)); // 初始化seraddr
    _addr.sin_family = AF_INET;

    // 本机字节序转换为网络字节序(包括ip与端口号)
    _addr.sin_port = htons(port);
    _addr.sin_addr.s_addr = inet_addr(ip.c_str()); // 把本机的ip转到网络
}

InetAddress::InetAddress(const struct sockaddr_in &addr)
    : _addr(addr)
{
}

InetAddress::~InetAddress()
{
}

string InetAddress::getIp()
{
    // intet_ntoa: 将一个32位的IPv4地址转换为 点分十进制的字符串
    return string(inet_ntoa(_addr.sin_addr));
}

unsigned short InetAddress::getPort()
{
    // ntohs: 将一个十六进制的数据 从网络字节序(大端)转换为主机字节序(小端)
    return ntohs(_addr.sin_port);
}

const struct sockaddr_in *InetAddress::getInetAddressPtr()
{
    return &_addr;
}
