#include "TcpConnection.h" // 包含 TcpConnection 的声明（类、成员声明等）
#include "EventLoop.h"     // 包含 EventLoop 的声明（用于 runInLoop 等）
#include <iostream>
#include <sstream>
using std::cout;
using std::endl;
using std::ostringstream;

// TcpConnection 的构造函数，参数：已建立连接的 fd，和所属的 EventLoop 指针
TcpConnection::TcpConnection(int fd, EventLoop *loop)
    : _loop(loop) // 把传入的 EventLoop 指针保存到成员 _loop（此对象所属的事件循环）
      ,
      _sockIO(fd) // 初始化 _sockIO（封装了对 socket 的读写操作），使用 fd 构造
      ,
      _sock(fd) // 初始化 _sock（封装 socket 操作的对象），使用 fd 构造
      ,
      _localAddr(getLocalAddr()) // 获取并保存本端地址（IP:port），在构造期间调用 getLocalAddr()
      ,
      _peerAddr(getPeerAddr()) // 获取并保存对端地址（IP:port），在构造期间调用 getPeerAddr()
{
    // 构造函数体为空（所有初始化都在初始化列表完成）
}

TcpConnection::~TcpConnection()
{
    // 析构函数为空：注意实际资源（如 fd）会由 _sock 的析构或别处负责关闭
}

// 进行一次“窥探式”(peek)读取，判断对端是否已经关闭连接
// 返回 true 表示对端已经关闭（recv 返回 0）
bool TcpConnection::isClosed()
{
    char buff[5] = {0};                                            // 临时缓冲区，用于 peek（不真正消费数据）
    int ret = ::recv(_sock.getFd(), buff, sizeof(buff), MSG_PEEK); // 使用 recv(..., MSG_PEEK) 预读，不从接收队列移除数据
    return 0 == ret;                                               // 如果 recv 返回 0 -> 对端关闭（TCP FIN）
}

// 从连接中接收一行文本并返回（基于 _sockIO.readLine 的封装）
Message TcpConnection::receive()
{
    Message msg;
    int ret = _sockIO.readMessage(msg); // readMessage 是我们前面写的函数
    std::cout << "在 Tcpconnection中receive ret = " << ret << endl;
    if (ret <= 0)
    {
        msg.tag = -1; // 标记为无效消息
        // 主动关闭连接
        ::close(_sock.getFd());
    }
    
    return msg; // 返回完整的 TLV 消息
}

// 立即在当前线程/上下文中发送数据（阻塞直到写完或出错）
void TcpConnection::send(const string &msg)
{
    _sockIO.writen(msg.c_str(), msg.size()); // 使用封装的 writen 保证写完整个 msg（注意类型转换：size_t->int）
}

// 将发送任务放到所属的 EventLoop 中执行（线程安全的跨线程发送）
void TcpConnection::sendInLoop(const string &msg)
{
    if (_loop) // 如果有绑定的事件循环（正常情况应该存在）
    {
        _loop->runInLoop(bind(&TcpConnection::send, this, msg)); // 绑定成员函数 send，并把 msg 复制进去，交给 EventLoop 执行
        // 解释：bind 会拷贝 msg → 如果 msg 很大会有拷贝开销；可以考虑用 move 或者封装一个移动的 lambda
    }
}

// 格式化并返回“本端IP:port =====> 对端IP:port”的字符串，便于打印日志
string TcpConnection::toString()
{
    ostringstream oss;                        // 临时 ostringstream 用于格式化字符串
    oss << _localAddr.getIp() << ":"          // 插入本地 IP
        << _localAddr.getPort() << " =====> " // 插入本地端口和分隔符
        << _peerAddr.getIp() << ":"           // 插入对端 IP
        << _peerAddr.getPort();               // 插入对端端口

    return oss.str(); // 返回格式化后的字符串
}

// 获取本地（socket 绑定/连接时的本地）地址（IP + port）
InetAddress TcpConnection::getLocalAddr()
{
    struct sockaddr_in addr;                                              // 用于接收地址信息的结构
    socklen_t len = sizeof(sockaddr);                                     // 这里建议改为 sizeof(sockaddr_in)，当前写法是个常见瑕疵
    int ret = getsockname(_sock.getFd(), (struct sockaddr *)&addr, &len); // getsockname 填充本端地址
    if (ret == -1)
    {
        perror("getsockname"); // 如果失败，打印错误（但仍继续返回 InetAddress(addr)）
    }

    return InetAddress(addr); // 用 addr 构造并返回 InetAddress
}

// 获取对端（peer）地址（IP + port）
InetAddress TcpConnection::getPeerAddr()
{
    struct sockaddr_in addr;                                              // 用于接收对端地址信息
    socklen_t len = sizeof(sockaddr);                                     // 同上，建议使用 sizeof(sockaddr_in)
    int ret = getpeername(_sock.getFd(), (struct sockaddr *)&addr, &len); // getpeername 填充对端地址
    if (ret == -1)
    {
        perror("getpeername"); // 错误打印（但仍返回 InetAddress(addr)，可能是未初始化的数据）
    }

    return InetAddress(addr); // 用 addr 构造并返回 InetAddress
}

// 以下三函数用于设置回调（由外部，如 Server 或上层业务，传入）
// 设置“新连接到来”回调（注意传入 const &，不使用移动语义）
void TcpConnection::setNewConnectionCallback(const TcpConnectionCallback &cb)
{
    // 此处不能使用移动语义（std::move），否则 EventLoop 的回调可能被置空（因为回调被移动走）
    /* _onNewConnection = std::move(cb); */ // 注释：故意不用移动
    _onNewConnection = cb;                  // 复制回调对象来保存
}

// 设置“有消息到来”回调
void TcpConnection::setMessageCallback(const TcpConnectionCallback &cb)
{
    _onMessage = cb; // 复制保存
}

// 设置“连接关闭”回调
void TcpConnection::setCloseCallback(const TcpConnectionCallback &cb)
{
    _onClose = cb; // 复制保存
}

// 以下三函数用于在合适时机触发回调（将 shared_ptr 本对象传给回调）
// 触发“新连接”回调：若回调存在，则调用并传入 shared_from_this()
void TcpConnection::handleNewConnectionCallback()
{
    if (_onNewConnection) // 检查回调是否被设置
    {
        // 需要传入的参数是一个管理本对象的 shared_ptr 智能指针
        // 要求 TcpConnection 必须继承 enable_shared_from_this<TcpConnection>
        _onNewConnection(shared_from_this()); // 使用 shared_from_this() 安全地生成 shared_ptr 并传递
    }
    else
    {
        cout << "_onNewConnection == nullptr" << endl; // 未设置回调时的提示
    }
}

// 触发“有消息到来”回调
void TcpConnection::handleMessageCallback()
{
    if (_onMessage)
    {
        _onMessage(shared_from_this()); // 传入 shared_ptr 以便回调持有对象
    }
    else
    {
        cout << "_onMessage == nullptr" << endl; // 未设置回调时提示
    }
}

// 触发“连接关闭”回调
void TcpConnection::handleCloseCallback()
{
    if (_onClose)
    {
        _onClose(shared_from_this()); // 传入 shared_ptr，让业务层可以安全地延长对象生命周期
    }
    else
    {
        cout << "_onClose == nullptr" << endl; // 未设置回调时提示
    }
}
