#include "EchoServer.h"
#include "MyTask.h"

// EchoServer 构造函数
// 参数：线程池数量 threadNum、队列大小 queSize、监听 IP 和端口
// 初始化线程池 _pool 和底层 TcpServer _server
EchoServer::EchoServer(size_t threadNum, size_t queSize,
                       const string &ip, unsigned short port)
    : _pool(threadNum, queSize) // 创建线程池
      ,
      _server(ip, port) // 创建 TcpServer
{
}

// EchoServer 析构函数
// 资源由成员自动析构，这里不需要手动释放
EchoServer::~EchoServer()
{
}

// 启动 EchoServer
void EchoServer::start()
{
    // 1. 启动线程池
    _pool.start();

    // 2. 使用 std::bind 将成员函数绑定到 TcpServer 的回调接口
    // _1 是 std::placeholders，表示回调时传入的第一个参数
    using namespace std::placeholders;

    // 设置 TcpServer 的三个核心回调函数：
    // - 新连接回调 -> onNewConnection
    // - 消息到达回调 -> onMessage
    // - 连接关闭回调 -> onClose
    _server.setAllCallback(
        bind(&EchoServer::onNewConnection, this, _1),
        bind(&EchoServer::onMessage, this, _1),
        bind(&EchoServer::onClose, this, _1));

    // 3. 启动 TcpServer（内部会调用 Acceptor 监听 + EventLoop 循环）
    _server.start();
}

// 停止 EchoServer
void EchoServer::stop()
{
    // 先停止线程池
    _pool.stop();

    // 再停止 TcpServer 的事件循环和监听
    _server.stop();
}

// 新连接到来时的回调函数
// 参数：TcpConnectionPtr 是 shared_ptr 封装的连接对象
void EchoServer::onNewConnection(const TcpConnectionPtr &con)
{
    // 打印连接信息
    cout << con->toString() << " has connected!!! " << endl;
}

// 客户端发送消息时的回调
void EchoServer::onMessage(const TcpConnectionPtr &con)
{
    cout << "客户端发送消息时的回调" << endl;
    // 1. 读取客户端发来的消息
    Message msg = con->receive();
    cout << "EchoServer. onMessage Messages->tag = " << msg.tag << endl;

    // 2. 打印消息
    // cout << ">> recv msg from client: " << msg << endl;

    // 3. 创建一个任务对象 MyTask（封装处理逻辑）
    MyTask task(msg, con);

    // 4. 将任务添加到线程池，由线程池异步处理
    _pool.addTask(bind(&MyTask::process, task));
    cout << "将任务添加进任务队列成功" << endl;
    cout << "重新看一下msg: " << msg.tag << "  length: " << msg.length << "msg.value  : " << msg.value << endl;
}

// 连接关闭时的回调
void EchoServer::onClose(const TcpConnectionPtr &con)
{
    // 打印关闭连接信息
    cout << con->toString() << " has closed!!! " << endl;
}
