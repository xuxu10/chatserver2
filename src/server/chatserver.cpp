#include "../../include/server/chatserver.hpp"
#include "../../include/server/chatservice.hpp"
#include "../../thirdparty/json.hpp"
#include <string>
#include <functional>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;//将nlohmann::json命名为json

//ChatServer类作用域下的ChatServer函数
ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
    //注册链接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));
    //读写消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));
    //设置线程数量 1个I/O 3个工作线程 主reactor负责用户的连接 子reactor负责用户的读写
    _server.setThreadNum(4);
}
//启动服务
void ChatServer::start()
{
    _server.start();
}
// 用户注册
void ChatServer::onConnection(const TcpConnectionPtr & conn)
{
    //客户端断开连接
    if(!conn -> connected()){
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}
// 已注册用户的读写
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer * buffer,
                           Timestamp time)
{
    string buf = buffer->retrieveAllAsString();
    //数据反序列化
    json js = json ::parse(buf);
    //达到的目的：完全解耦网络模块的代码和业务模块的代码
    //通过js["msgid"]获取=》业务处理器handler=》conn js time通过处理器传出
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());//js["msgid"]是json类型的数据，采用get<int>()方法转为整型数据
    //回调消息绑定好的事件处理器，来执行相应的业务处理
    msgHandler(conn,js,time);

}