#ifndef CHATSERVICE_H//防止头文件重复包含
#define CHATSERVICE_H
#include <unordered_map>//内部实现了一个哈希表元素的排列顺序是无序;map红黑树有顺序要求
#include <functional>
#include <muduo/net/TcpConnection.h>
#include "../../thirdparty/json.hpp"
#include <mutex>//c++11多线程：mutex是可锁定的类型，用来保护临界区代码
#include "../server/model/usermodel.hpp"
#include "../server/model/offlinemessagemodel.hpp"
#include "../server/model/friendmodel.hpp"
#include "../server/model/groupmodel.hpp"
#include "redis/redis.hpp"
using namespace muduo;
using namespace muduo::net;
using namespace std;
using  json = nlohmann::json;

//表示处理消息的事件回调方法的类型
using MsgHandler = std::function<void(const TcpConnectionPtr &conn,json &js,Timestamp )>;
//聊天服务器业务类（采用单例模式实现） 主要与业务有关，有一个实例就够用了
class ChatService{
public:
    //获取单例对象的接口函数
    static ChatService* instance();
    //处理登录业务
    void login(const TcpConnectionPtr& conn,json &js,Timestamp time);
    //处理注册业务
    void reg(const TcpConnectionPtr& conn,json &js,Timestamp time);
    //处理一对一聊天业务
    void oneChat(const TcpConnectionPtr& conn,json &js,Timestamp time);
    //添加好友业务
    void addFriend(const TcpConnectionPtr& conn,json &js,Timestamp time);
    //创建群组业务
    void creatGroup(const TcpConnectionPtr& conn,json &js,Timestamp time);
    //加入群组业务
    void addGroup(const TcpConnectionPtr& conn,json &js,Timestamp time);
    //发起群聊
    void groupChat(const TcpConnectionPtr& conn,json &js,Timestamp time);
    //退出业务处理
    void quit(const TcpConnectionPtr& conn,json &js,Timestamp time);
    //获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    //处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);
    //服务器异常时 重置状态
    void reset();
    //从redis消息队列中获取订阅的消息
    void handlerRedisSubscribeMessage(int,string);

private:
    //单例模式 首先构造函数私有化
    ChatService();
    //存储消息id和其对应的业务处理方法
    unordered_map<int,MsgHandler>_msgHandlerMap;

    //存储在线用户的通信连接 要注意线程安全
    unordered_map<int,TcpConnectionPtr> _userConnMap;

    //定义互斥锁，保证_userConnMap的线程安全
    mutex _connMutex;

    //数据操作类对象  业务层不直接进行数据库的更改 依赖于_userModle ，_userModle给业务层提供的是对象
    UserModel _userModle;
    //操作离线消息
    OfflineMsgModel _offlineMsgModel;

    //存储好友信息
    FriendModel _friendModel;

    //操作群聊
    GroupModel _groupModel;

    //redis操作对象
    Redis _redis;

    

};


#endif