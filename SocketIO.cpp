#include "SocketIO.h"

SocketIO::SocketIO(int fd)
    : _fd(fd)
{
}

SocketIO::~SocketIO()
{
    /* close(_fd); */
}
int SocketIO::readMessage(Message &msg)
{
    // 读取头部 (8字节)
    int header[2] = {0};
    int headerSize = sizeof(header);
    int totalRead = 0;

    while (totalRead < headerSize)
    {
        int ret = readn((char *)header + totalRead, headerSize - totalRead);
        if (ret <= 0)
            return -1; // 错误或对端关闭
        totalRead += ret;
    }

    msg.tag = header[0];
    msg.length = header[1];

    // 读取变长数据
    if (msg.length > 0)
    {
        std::vector<char> buffer(msg.length + 1, 0);
        totalRead = 0;
        while (totalRead < msg.length)
        {
            int ret = readn(buffer.data() + totalRead, msg.length - totalRead);
            if (ret <= 0)
                return -1;
            totalRead += ret;
        }
        msg.value.assign(buffer.data(), msg.length);
    }
    std::cout << "构建msg成功 tag= " << msg.tag << "length = " << msg.length << " value = " << msg.value << std::endl;
    return headerSize + msg.length;
}
int SocketIO::readn(char *buf, int len) // 10000
{
    // 设置的任务可能是读取很多个字符
    // 也许会超过一次read能够读取的上限
    //
    // 比如要读取10000个字符，但是单次read只能读1000
    int left = len;   // 10000
    char *pstr = buf; // 读缓存区的首地址
    int ret = 0;

    // 因为read函数单次读取的数据量存在上限
    while (left > 0)
    {
        ret = ::read(_fd, pstr, left);
        // 对当前进程（线程）而言发生了中断
        // 中断触发，导致CPU的使用权被抢夺，后续应该继续执行
        if (-1 == ret && errno == EINTR)
        {
            // 信号中断
            continue;
        }
        else if (-1 == ret)
        {
            // 真正的错误
            perror("read error -1");
            return -1;
        }
        else if (0 == ret)
        {
            // 对端关闭
            break;
        }
        else
        {
            // 实际读到了ret个字节
            pstr += ret; // 读缓冲区放了第一次读到的1000个字符，然后指针偏移
            left -= ret; // 10000 - 1000 = 9000  left是剩余要读取的字符数
        }
    }
    std::cout << "readn: expect=" << len << " actual=" << (len - left) << std::endl;
    // 返回实际读到的字节数
    return len - left;
}

// 可靠地从套接字中按行读取数据
int SocketIO::readLine(char *buf, int len)
{
    int left = len - 1; // 预留一个字节 放0
    char *pstr = buf;   // 当前写入缓冲区的位置
    int ret = 0, total = 0;

    while (left > 0)
    {
        // MSG_PEEK不会将缓冲区中的数据进行清空,只会进行拷贝操作. "peek":偷看
        ret = recv(_fd, pstr, left, MSG_PEEK);
        if (-1 == ret && errno == EINTR)
        {
            // 信号中断
            continue;
        }
        else if (-1 == ret)
        {
            // 真正的错误
            perror("readLine error -1");
            return -1;
        }
        else if (0 == ret)
        {
            // 对端关闭
            break;
        }
        else
        {
            // 在ret个字节中找有没有换行符
            for (int idx = 0; idx < ret; ++idx)
            {
                // 找到了换行符
                if (pstr[idx] == '\n')
                {
                    int sz = idx + 1; // 一行的长度
                    readn(pstr, sz);  // 真正的读走这行数据
                    pstr += sz;       // 移动写入位置指针
                    *pstr = '\0';     // C风格字符串以'\0'结尾

                    return total + sz; // 返回总共读到的字节数
                }
            }
            // 没有找到换行符
            readn(pstr, ret); // 把peek的数据真正的取走
            total += ret;     // 已经累计的字节数
            pstr += ret;      // 缓冲区指针后移
            left -= ret;      // 更新剩余可读空间
        }
    }
    // 没有读取到换行符,需要保证字符串安全地结束
    *pstr = '\0';

    // 返回已读的字节数
    return total;
}

// 可靠地将数据写出去(文件描述符)
int SocketIO::writen(const char *buf, int len)
{
    int left = len;
    const char *pstr = buf;
    int ret = 0;

    // 同样，一次write能写的数据量也有上限
    while (left > 0)
    {
        ret = write(_fd, pstr, left);
        if (-1 == ret && errno == EINTR)
        {
            continue;
        }
        else if (-1 == ret)
        {
            perror("writen error -1");
            return -1;
        }
        else if (0 == ret)
        {
            break;
        }
        else
        {
            pstr += ret;
            left -= ret;
        }
    }
    return len - left;
}
