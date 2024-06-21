#include "../../include/server/chatservice.hpp"
#include "../../include/public.hpp"
#include "../../include/server/redis/redis.hpp"
#include "../../include/server/redis/redis.hpp"
#include <muduo/base/Logging.h>
#include <string>
#include <vector>
#include <iostream>
using namespace std;
using namespace muduo;
/**
 * json数据
 * userid name state msgid msg
 * friendid 一对一聊天中friendid=to
 * groupid groupname groupdesc grouprole groups所有群 users群中的所有用户
 */
//获取单例对象的接口函数
ChatService* ChatService::instance(){//静态方法在类外实现不用写static
    static ChatService service;
    return &service;
}

//实现构造函数 注册消息以及对应的回调Handler
ChatService::ChatService(){
    _msgHandlerMap.insert({LOGIN_MSG,std::bind(&ChatService::login,this,_1,_2,_3)});
    _msgHandlerMap.insert({REG_MSG,std::bind(&ChatService::reg,this,_1,_2,_3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG,std::bind(&ChatService::oneChat,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG,std::bind(&ChatService::addFriend,this,_1,_2,_3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG,std::bind(&ChatService::creatGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG,std::bind(&ChatService::addGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG,std::bind(&ChatService::groupChat,this,_1,_2,_3)});
    _msgHandlerMap.insert({LOGINOUT_MSG,std::bind(&ChatService::quit,this,_1,_2,_3)});
    
    //连接redis服务器
    if(_redis.connect()){
        //设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handlerRedisSubscribeMessage,this,_1,_2));
    }
}
//获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid){
    //记录错误日志 msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if(it == _msgHandlerMap.end()){//在map中没有找到msgid
        
       //返回一个空操作
       return [=](const TcpConnectionPtr &conn,json &js,Timestamp){// lambda表达式（也称为lambda函数）是在调用或作为函数参数传递的位置处定义匿名函数对象的便捷方法。
        //[=]按照副本引用this，还有当前块语句所有的局部变量，不可以赋值，但是可以读取
        //[&]表示lambda 中的 msgid 是对封闭范围内的 msgid 的引用

            //使用muduo库的日志记录 不用写endl 该日志会自动换行
            LOG_ERROR<<"msgid:"<<msgid<<" can not find handler!";//找不到相应额处理器
       };
    }
    else{
        return _msgHandlerMap[msgid];//map id键得到对应的map值
    }
   
}
 //服务器异常时 重置状态
void ChatService::reset(){
    //将所有online的状态变为offline
    _userModle.resetState();
}
//处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn){
    User user;
    {//定义一个互斥锁，防止在删除连接时有用户正在读写
        lock_guard<mutex> lock(_connMutex);
        
        // 1.按照智能指针搜索表,找到键值对，从表中删除
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (conn == it->second)
            {
                // 记录key
                user.setId(it->first);
                // 删除用户连接信息
                _userConnMap.erase(it);
                break;
            }
        }
    }
     //用户注销，相当于下线，在redis中取消订阅通道
    _redis.unsubscribe(user.getId());

    //2.根据键值对的id将用户状态online修改为offline
    if(user.getId() != -1){
        user.setState("offline");
        _userModle.updateState(user);
    }
    
}
//退出
void ChatService::quit(const TcpConnectionPtr& conn,json &js,Timestamp time){
    int userid = js["userid"].get<int>();
    {//定义一个互斥锁，防止在删除连接时有用户正在读写
        lock_guard<mutex> lock(_connMutex);
        
        // 1.按照智能指针搜索表,找到键值对，从表中删除
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {

            // 删除用户连接信息
            _userConnMap.erase(it);
        }
    }
    //用户注销，相当于下线，在redis中取消订阅通道
    _redis.unsubscribe(userid);
        //将用户状态online修改为offline
        User user(userid,"","","offline");
        _userModle.updateState(user);
        
    
}
//处理登录业务 ORM 对象关系映射框架：业务层操作的都是对象 数据层具体的数据库的操作
void ChatService::login(const TcpConnectionPtr& conn,json &js,Timestamp time){
    //通过日志打印
    //LOG_INFO<<"do login service!!";
    int userid = js["userid"];
    string pwd = js["password"];

    User user = _userModle.query(userid);
    if(user.getId() == userid && user.getPwd() == pwd){
        if(user.getState() == "online"){
            //该用户已经登录不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 1;
            response["errmsg"] = "该账号已经登录，请重新输入新账号";
            conn->send(response.dump());
        }
        else{
            //登陆成功，记录用户连接信息  多个用户同时登录会出现线程拥堵 要添加线程互斥操作
           {
                //锁的力度要小 在大括号开始加上锁，大括号结束解锁
                lock_guard<mutex> lock(_connMutex); //lock_guard<>模板类 ：构造函数加锁 析构函数解锁 引用接受
                _userConnMap.insert({userid,conn});
           }
           //id用户登录成功后，向redis订阅channel（userid）如果有给userid发送的消息，redis就会上报通知
           _redis.subscribe(userid);
           cout<<"subscribe"<<endl;
          

            //登录成功,更新用户的状态信息 state offline ==> online
            user.setState("online");
            _userModle.updateState(user);
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;//错误编号error number如果为0表示没有错误，如果为1表示出错
            response["userid"] = user.getId();
            response["name"] = user.getName();
           
            //查找离线表中是否有该用户的离线消息
            vector<string> vec = _offlineMsgModel.query(userid);
            if(!vec.empty()){
                //如果离线消息不为空
                response["offlinemsg"] = vec;//json可以直接显示vector内容
               //读取完离线消息后，把该用户的离线消息删除
                _offlineMsgModel.remove(userid);
            }

            // //登录成功返回好友列表 自己写的
            // vector<User> userVec = _friendModel.query(id);
            // if(!vec.empty()){
            //     //如果好友信息不为空
            //     for (auto it = userVec.begin(); it != userVec.end(); ++it)
            //     {
            //         response["friendid"] = it->getId();
            //         response["friendname"] = it->getName();
            //         response["friendstate"] = it->getState();
            //     }
            // }

            //登录成功返回好友列表
            vector<User> userVec = _friendModel.query(userid);
            if(!userVec.empty()){
                //如果好友信息不为空
                vector<string> vec2;
                for (User &user :userVec )
                {
                    json js;
                    js["userid"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
                //LOG_INFO<<"send friends!";
            }   
             //登录成功返回群组信息列表
            vector<Group> groupVec = _groupModel.queryGroups(userid);
            if(!groupVec.empty()){
                //如果群组信息不为空
                vector<string> gVec;
                for (Group &group : groupVec )
                {
                    json js;
                    js["groupid"] = group.getId();
                    js["groupname"] = group.getName();
                    js["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for(GroupUser&guser : group.getUsers()){
                        json groupUserjs;
                        groupUserjs["userid"] = guser.getId();
                        groupUserjs["name"] = guser.getName();
                        groupUserjs["state"] = guser.getState();
                        groupUserjs["grouprole"] = guser.getRole();
                        userV.push_back(groupUserjs.dump());
                    }
                    js["users"] = userV;
                    gVec.push_back(js.dump());
                }
                response["groups"] = gVec;
                //LOG_INFO<<"send friends!";
            } 
            //cout << "chatserver "<<response.dump() << endl;  //grouprole\\\":\\\"\\\"
            conn->send(response.dump());
        }
        
    }
    else{//登录失败
        
        if(user.getId() == -1){
            //该用户不存在
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "error:this user is not exist!";
            conn->send(response.dump());
        }
        else{
            //用户名或者密码错误
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 3;
            response["errmsg"] = "error:this userid or password is wrong!";
            conn->send(response.dump());
        }
        
    }

}
//处理注册业务 name password
void ChatService::reg(const TcpConnectionPtr& conn,json &js,Timestamp time){
    //LOG_INFO<<"do reg service!!";
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModle.insert(user);
    if(state){
        //注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;//错误编号error number如果为0表示没有错误，如果为1表示出错
        response["userid"] = user.getId();
        conn->send(response.dump());
    }
    else{
        //注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

//处理一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr& conn,json &js,Timestamp time){
    int toid = js["to"].get<int>(); //get<int>()转为int类型
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if(it != _userConnMap.end()){
            //toid用户在线，在_userConnMap在线用户表中找到
            //用户在线转发消息 服务器主动推送消息给toid用户
            it->second->send(js.dump());
            return;
        }
        else{
            // 如果在在线用户表中没有找到to用户，有可能在别的服务器上
            // 查询to用户是否在线
            User user = _userModle.query(toid);
            if (user.getState() == "online")
            {
                _redis.publish(toid, js.dump());
                cout << "publish" << endl;
                return;
            }
            else{
                // 用户不在线，存储消息  js.dump()将json数据转为字符串
               _offlineMsgModel.insert(toid, js.dump());
            }

            
        }
    }
    
}
/**toid用户在线 测试
 * 新注册用户li si给在线用户zhangsan发消息
 * {"msgid":2,"name":"li si","password":"111"}注册，返回id号3
 * 
 * {"msgid":1,"userid":3,"password":"111"}lisi登录，修改状态为online
 * {"msgid":1,"userid":1,"password":"123456"}zhangsan登录
 * {"msgid":5,"id":3,"from":"li si","to":1,"msg":"hello"}lisi发消息
 * //zhangsan收到的消息：{"from":"li si","id":3,"msg":"hello","msgid":5,"to":1}
 * {"msgid":5,"id":1,"from":"zhangsan","to":3,"msg":"hello!!!"}zhangsan发消息
 * //lisi收到的消息：{"from":"zhangsan","id":1,"msg":"hello!!!","msgid":5,"to":3}
 * 
 */

//添加好友业务 msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr& conn,json &js,Timestamp time){
    //这里的js是：
    int userid = js["userid"].get<int>();
    int friendid = js["friendid"].get<int>();
    /**添加好友信息
     * {"msgid":6,"userid":1,"friendid":2}
     */
    //存储好友信息
    _friendModel.insert(userid,friendid);
}



//创建群组业务
void ChatService::creatGroup(const TcpConnectionPtr& conn,json &js,Timestamp time){
    //读取js
    int userid = js["userid"].get<int>();
    string groupname = js["groupname"];
    string groupdesc = js["groupdesc"];
    Group group;
    group.setName(groupname);
    group.setDesc(groupdesc);

    //创建群组 如果创建成功 存储创建人信息
    if(_groupModel.createGroup(group)){
        _groupModel.addGroup(userid,group.getId(),"creator");
        json response;
        response["msgid"] = CREATE_GROUP_MSG;
        response["errno"] = 0;//成功
        response["groupid"] = group.getId();
        conn->send(response.dump());
    }
}
//加入群组业务
void ChatService::addGroup(const TcpConnectionPtr& conn,json &js,Timestamp time){
    //读取js
    int userid = js["userid"].get<int>();
    int groupid = js["groupid"].get<int>();
    //加入群组的用户不属于创建者 所以只能是normalstring role = js["grouprole"];

    //加入群组
    _groupModel.addGroup(userid,groupid,"normal");
    json response;
    response["msgid"] = ADD_GROUP_MSG;
    response["errno"] = 0;//成功
    conn->send(response.dump());
}
//发起群聊
void ChatService::groupChat(const TcpConnectionPtr& conn,json &js,Timestamp time){
    //读取js
    int userid = js["userid"].get<int>();
    int groupid = js["groupid"].get<int>();
    //返回整个js消息 不需要单独读取string msg = js["msg"];
    //返回一个vector<int>里面是需要发送消息的id
    vector<int> idVec = _groupModel.queryGroupUsers(userid,groupid);
    //循环还可以
    /**
     * for(int id : idVec){
     *  lock_guard<mutex> lock(_connMutex);
            auto it = _userConnMap.find(id);
            .....
     * }
     */
    lock_guard<mutex> lock(_connMutex);//避免多次加锁解锁
    for(auto It = idVec.begin();It != idVec.end();It++){
        
            
            auto it = _userConnMap.find(*It);
            if (it != _userConnMap.end())
            {
                // toid用户在线，在_userConnMap在线用户表中找到
                // 用户在线转发消息 服务器主动推送消息给toid用户
                it->second->send(js.dump());
            }
            else{
                // 如果在在线用户表中没有找到to用户，有可能在别的服务器上
                // 查询to用户是否在线
                User user = _userModle.query(*It);
                if (user.getState() == "online")
                {
                    _redis.publish(*It, js.dump());
                    cout << "publishgroup" << endl;
                    return;
                }
                else
                {
                    // 用户不在线，存储消息  js.dump()将json数据转为字符串
                    _offlineMsgModel.insert(*It, js.dump());
                }
            }

            

        
    }
}

//从redis消息队列中获取订阅的消息
void  ChatService::handlerRedisSubscribeMessage(int userid,string msg){
   // json js = json::parse(msg.c_str());
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if(it != _userConnMap.end()){//该用户在线，找到他的conn
        //it->second->send(js.dump());
        it->second->send(msg);
        return;
    }

    //存储该用户的离线消息
    _offlineMsgModel.insert(userid,msg);
    
}