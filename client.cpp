#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <strings.h>
#include <vector>
#include <unistd.h>
using namespace std;
struct Message
{
    int tag;
    int length;
    std::string value;
};

int tcp_connect(const char *host, const char *service)
{
    struct addrinfo hints, *res, *p;
    int sockfd;

    bzero(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     // 不限定 IPv4 或 IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP

    int ret = getaddrinfo(host, service, &hints, &res);
    if (ret != 0)
    {
        std::cerr << "getaddrinfo error: " << gai_strerror(ret) << "\n";
        return -1;
    }

    for (p = res; p != nullptr; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
            continue;

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == 0)
        {
            freeaddrinfo(res);
            return sockfd; // 成功返回连接描述符
        }

        close(sockfd);
    }

    freeaddrinfo(res);
    return -1; // 没连上
}

void send_message(int connfd, const Message &msg)
{
    std::vector<char> buffer;
    buffer.reserve(sizeof(msg.tag) + sizeof(msg.length) + msg.length);

    // 头部
    const char *p = reinterpret_cast<const char *>(&msg.tag);
    buffer.insert(buffer.end(), p, p + sizeof(msg.tag));

    p = reinterpret_cast<const char *>(&msg.length);
    buffer.insert(buffer.end(), p, p + sizeof(msg.length));

    // 主体
    buffer.insert(buffer.end(), msg.value.begin(), msg.value.end());

    // 单次发送
    send(connfd, buffer.data(), buffer.size(), 0);
}
ssize_t readn(int fd, void *buf, size_t n)
{
    size_t left = n;
    char *ptr = (char *)buf;
    while (left > 0)
    {
        ssize_t nread = recv(fd, ptr, left, 0);
        if (nread < 0)
        {
            if (errno == EINTR)
                continue; // 信号中断，继续读
            return -1;    // 其他错误
        }
        else if (nread == 0)
        {
            break; // 对端关闭
        }
        left -= nread;
        ptr += nread;
    }
    return (n - left);
}

int main()
{
    const char *host = "127.0.0.1";
    const char *service = "8888";
    int connfd = tcp_connect(host, service);
    if (connfd == -1)
    {
        std::cerr << "Error: connnect server failed!\n";
        exit(1);
    }

    for (;;)
    {
        std::cout << "message: <Tag><Value>\n";
        Message message;
        std::cin >> message.tag;
        std::cin >> message.value;
        message.length = message.value.size();

        send_message(connfd, message);

        // TODO: 接收服务器的响应
        Message resp;
        if (readn(connfd, &resp.tag, sizeof(resp.tag)) <= 0)
            break;
        if (readn(connfd, &resp.length, sizeof(resp.length)) <= 0)
            break;

        resp.value.resize(resp.length);
        if (readn(connfd, &resp.value[0], resp.length) <= 0)
            break;

        std::cout << "server response: "
                  << "tag=" << resp.tag
                  << ", value=" << resp.value << "\n";
    }
}