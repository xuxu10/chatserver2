#include "../../thirdparty/json.hpp"
#include "../../include/server/model/user.hpp"
//#include "../../include/server/model/friendmodel.hpp"
#include "../../include/server/model/group.hpp"
#include "../../include/public.hpp"
//#include <muduo/base/Logging.h>

#include <unistd.h>//close()
#include <sys/socket.h>//socket
#include <netinet/in.h>
#include <arpa/inet.h>//需要上面三个sockaddr_in
#include <sys/types.h>

#include <ctime>
#include <iostream>
//#include <fstream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <semaphore.h>//linux信号量
#include <atomic>//c++11原子类型 该变量在多个线程中读写保持原子特性

using json = nlohmann::json;
using namespace std;
//控制主菜单页面程序
bool isMainMenuRunning = false;
//定义用于读写线程之间的通信
sem_t rwsem;
//记录登录状态
atomic_bool g_isLoginSuccess{false};
//记录当前系统登录的用户信息
User g_currentUser;
//记录当前登录用户的好友信息列表
vector<User> g_currentUserFriends;
//记录当前用户所在的群组信息列表
vector<Group> g_currentUserGroupList;
//显示当前用户的基本信息
void showCurrentUserData();

//接收线程 发送和接收是并行的 
void readTaskHandler(int clientfd);
//获取系统时间（聊天信息需要添加时间信息）c++11封装好的方法
string getCurrentTime();
//主聊天页面程序
void mainMenu(int clientfd);

//聊天客户端程序实现，main线程用作发送线程，子线程用作接收线程
int main(int argc, char **argv)//argc命令的个数 *argv[]是个指针数组，存放输入在命令行上的命令（字符串）。
{
    if (argc < 3)//必须是这三个命令才正确./ChatClient 127.0.0.1 6000
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);//uint16_t无符号双字节

    // 创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        cerr << "socket create error" << endl;
        exit(-1);//非正常退出 exit(0)正常退出
    }

    // 填写client需要连接的server信息ip+port
    sockaddr_in server;//解决了sockaddr的缺陷，把port和addr 分开储存在两个变量中
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;//地址族，e.g. AF_INET:IPv4 网络协议的套接字类型, AF_INET6
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // client和server进行连接
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    //初始化读写线程通信用的信号量
    sem_init(&rwsem, 0, 0);

    //连接服务器成功，启动接收子线程
    std::thread readTask(readTaskHandler, clientfd); // pthread_create
    readTask.detach();                               // pthread_detach

    // main线程用于接收用户输入，负责发送数据
    for (;;)
    {
        // 显示首页面菜单 登录、注册、退出
        cout << "========================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "========================" << endl;
        cout << "choice:";
        int choice = 0;
        cin >> choice;
        cin.get(); // 读掉缓冲区残留的回车

        switch (choice)
        {
        case 1: // login业务
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "userid:";
            cin >> id;
            cin.get(); // 读掉缓冲区残留的回车
            cout << "userpassword:";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["userid"] = id;
            js["password"] = pwd;
            string request = js.dump();

            g_isLoginSuccess = false;

            //主线程只发送数据
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send login msg error:" << request << endl;
            }
            
            sem_wait(&rwsem);//等待信号量，有子线程处理完登录响应消息后，通知
            if(g_isLoginSuccess){
                // 进入聊天主菜单页面
                isMainMenuRunning = true;
                mainMenu(clientfd);  
            }   
        }
        break;
        case 2: // register业务
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username:";
            cin.getline(name, 50);
            cout << "userpassword:";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send reg msg error:" << request << endl;
            }
            sem_wait(&rwsem);//等待信号量，子线程处理完注册会通知
            
        }
        break;
        case 3: // quit业务
            close(clientfd);
            sem_destroy(&rwsem);//信号量资源释放

            exit(0);
        default:
            cerr << "invalid input!" << endl;
            break;
        }
    }

    return 0;
}

//处理登录响应逻辑 判断登陆成功还是失败
void doLoginResponse(json &responsejs){

    if (0 != responsejs["errno"].get<int>()) // 登录失败
    {                                        // 登录失败1.用户已在线 2.id或密码错误 3.用户不存在ID=-1 这三种情况导致errno不等于0
        cerr << responsejs["errmsg"] << endl;
        g_isLoginSuccess = false;

    }
    else
    { // 登录成功
        // 记录当前用户的id和name
        g_currentUser.setId(responsejs["userid"].get<int>());
        g_currentUser.setName(responsejs["name"]);

        // 记录当前用户的好友列表信息
        if (responsejs.contains("friends"))
        {
            // 初始化 避免下次登录数据重复
            g_currentUserFriends.clear();
            vector<string> vec = responsejs["friends"];
            for (string &str : vec)
            {
                json js = json::parse(str);
                User user;
                user.setId(js["userid"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserFriends.push_back(user);
            }
        }

        if (responsejs.contains("groups"))
        {
            // 初始化 避免下次登录数据重复
            g_currentUserGroupList.clear();
            vector<string> Gvec = responsejs["groups"];
            for (string &gstr : Gvec)
            {
                json Gjs = json::parse(gstr);
                Group group;
                group.setId(Gjs["groupid"].get<int>());
                group.setName(Gjs["groupname"]);
                group.setDesc(Gjs["groupdesc"]);
                // 出错代码区域 改正前vector<string> uservec = responsejs["users"]; 改正后vector<string> Uvec = Gjs["users"];
                vector<string> Uvec = Gjs["users"];
                for (string &userstr : Uvec)
                {

                    GroupUser user;
                    json userjs = json::parse(userstr);

                    user.setId(userjs["userid"].get<int>());

                    user.setName(userjs["name"]);

                    user.setState(userjs["state"]);
                    user.setRole(userjs["grouprole"]);

                    group.getUsers().push_back(user);
                }

                g_currentUserGroupList.push_back(group);
            }
        }

        // 显示登录成功用户的基本信息,好友信息，群组信息
        showCurrentUserData();
        // 显示当前用户的离线消息 个人聊天信息或群组消息
        if (responsejs.contains("offlinemsg"))
        {
            vector<string> vec = responsejs["offlinemsg"];
            for (string &str : vec)
            {
                json js = json::parse(str);
                // 消息形式：时间【id】姓名said：消息

                if (ONE_CHAT_MSG == js["msgid"].get<int>())
                {
                    cout << js["time"].get<string>() << "[" << js["userid"] << "]" << js["name"] << "said:" << js["msg"] << endl;
                }
                else
                {
                    cout << "groupchat[" << js["groupid"] << "]: " << js["time"].get<string>() << "[" << js["userid"] << "]" << js["name"] << "said:" << js["msg"] << endl;
                }
            }
        }
        g_isLoginSuccess = true;
    }
}
//处理注册响应逻辑 判断登陆成功还是失败
void doRegResponse(json &js){

    if (0 != js["errno"].get<int>())
    {
        cerr  << " register error" << endl;
    }
    else
    {
        cout << " register success, userid is " << js["userid"] << endl;
    }

            //sem_wait(&rwsem); // 等待信号量，子线程处理完注册消息会通知
}
//接收线程 发送和接收是并行的 
void readTaskHandler(int clientfd){
    for(;;){
        char buffer[1024] = {0};
        int len = recv(clientfd,buffer,1024,0);
        if(len == -1 || len == 0){
            close(clientfd);
            exit(-1);
        }
        //接收chatserver转发的数据，反序列化生成json数据对象
        json js = json::parse(buffer);
        int msgType = js["msgid"].get<int>();
        //输出处理实时消息
        if(ONE_CHAT_MSG == msgType ){
            cout<<js["time"].get<string>()<<"["<<js["userid"]<<"]"<<js["name"]<<"said:"<<js["msg"]<<endl;
            continue;
        }
        
        if(GROUP_CHAT_MSG == msgType){
            cout<<"groupchat["<<js["groupid"]<<"]: "<<js["time"].get<string>()<<"["<<js["userid"]<<"]"<<js["name"]<<"said:"<<js["msg"]<<endl;
            continue;
        } 
        if(LOGIN_MSG_ACK == msgType){
            doLoginResponse(js);//处理登录响应的业务逻辑
            sem_post(&rwsem);//通知主线程登录处理完成
            continue;
        }
        if(REG_MSG_ACK == msgType){
            doRegResponse(js);
            sem_post(&rwsem);//通知主线程登录处理完成
            continue;
        }

    }

}
//命令函数声明
//hlep command handler
void help(int fd = 0,string str = " ");
//chat command handler
void chat(int,string);
//addfriend command handler
void addfriend(int,string);
//creategroup command handler
void creategroup(int,string);
//addgroup command handler
void addgroup(int,string);
//groupchat command handler
void groupchat(int,string);
//quit command handler
void quit(int,string);

//命令的解释
unordered_map<string,string> commandInfoMap ={
    {"help","显示所有支持的命令,格式:help"},
    {"chat","一对一聊天,格式:chat:id:message"},
    {"addfriend","添加好友,格式:addfriend:id"},
    {"creategroup","创建群组,格式:ctreateGroup:groupname:groupmsg"},
    {"addgroup","加入群组,格式:addgroup:groupid"},
    {"groupchat","发起群聊,格式:groupchat:groupid:message"},
    {"quit","退出,格式:quit"}
};

//注册系统支持的客户端命令处理 function<void(int,string)>:命令处理函数的指向函数，这些命令函数都有两个输入参数客端fd和输入消息
unordered_map<string,function<void(int,string)>> commandHandlerMap ={
    {"help",help},
    {"chat",chat},
    {"addfriend",addfriend},
    {"creategroup",creategroup},
    {"addgroup",addgroup},
    {"groupchat",groupchat},
    {"quit",quit}
};
//主聊天页面程序
void mainMenu(int clientfd){
    help();
    char buffer[1024] = {0};
    while (isMainMenuRunning)
    {
         cin.getline(buffer,1024);
        string commandbuf(buffer);
        string command;//存储命令
        int index = commandbuf.find(":");//第一个冒号之前是命令 之后是消息
        if(-1 == index){//没找到冒号
            command = commandbuf;//help quit
        }
        else{
            command = commandbuf.substr(0,index);//substr求子串 第一个是起始位置 第二个是串长度
        }
        auto it = commandHandlerMap.find(command);
        if(it == commandHandlerMap.end()){
            cerr <<"invalid input command"<<endl;
            continue;
        }
        
        //it->second是命令函数
        it->second(clientfd,commandbuf.substr(index+1,commandbuf.size()-index));
    }
    
}
//命令函数实现
//help command handler
void help(int,string){
    cout<<"show command list>>"<<endl;
    for(auto &p:commandInfoMap){
        cout<<p.first<<":"<<p.second<<endl;
    }
    cout<<endl;
}
//chat command handler
void chat(int clientfd,string str){
    //LOG_INFO<<" chat 1 success!";
    //cout<<" chat 1 success!"<<endl;
    //str的内容是friendid:msg 将两个信息按照冒号获取
    int index = str.find(":");
    if(-1 == index){
        cerr<<"chat command invalid!"<<endl;
        return;
    }
    int friendid = atoi(str.substr(0,index).c_str());
    string message = str.substr(index+1,str.size()-index);
    //LOG_INFO<<" chat 2 success!"<<friendid<<message;
    //cout<<friendid<<" "<<message<<endl;
    //序列化成json发送
    //一对一聊天发送内容： msgid 发送人id 姓名name  toid 内容 时间
    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["userid"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["to"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();
    //LOG_INFO<<" chat 3 send buffer success!";
    //cout<<" chat 3 send buffer success!"<<endl;
    int len = send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1 == len){
        cerr<<"send chat msg error ->"<<endl;
    }
}
//addfriend command handler
void addfriend(int clientfd,string str){
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["friendid"] = friendid;
    js["userid"] = g_currentUser.getId();
    string buffer = js.dump();

    int len = send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1 == len){
        cerr<<"send addfriend msg error ->"<<endl;
    }
}
//creategroup command handler
void creategroup(int clientfd,string str) {
    //str ::  groupname:groupmsg
     
    int index = str.find(":");
    if(-1 == index){
        cerr<<"chat command invalid!"<<endl;
        return;
    }
    string groupname = str.substr(0,index).c_str();
    string groupdesc = str.substr(index+1,str.size()-index);
    
    //序列化成json发送
    //创建群组： msgid groupname groupdesc  ；客户端只管发送信息，服务端执行操作把该群加入当前群列表；把创建人加入群聊中
    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["userid"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();

    int len = send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1 == len){
        cerr<<"send creategroup msg error ->"<<endl;
    }
    else{
        char buffer[1024] = {0};
        len = recv(clientfd, buffer, 1024, 0);
        if (-1 == len)
        {
            cerr << "recv creategroup response error" << endl;
        }
        else
        {
            json js = json::parse(buffer);
            if (0 != js["errno"].get<int>())
            {
                cerr << groupname << " creategroup error" << endl;
            }
            else
            {   cout << "chatserver "<<js.dump() << endl;
                cout << groupname << " creategroup success, groupid is " << js["groupid"] << endl;
            }
        }
    }


}
//addgroup command handler
void addgroup(int clientfd,string str){
    //str：：groupid
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["userid"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();

    int len = send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1 == len){
        cerr<<"send addgroup msg error ->"<<endl;
    }
}
//groupchat command handler
void groupchat(int clientfd,string str){
    //groupid:message
    int index = str.find(":");
    if(-1 == index){
        cerr<<"groupchat command is invalid!"<<endl;
    }
    int groupid = atoi(str.substr(0,index).c_str());
    string message = str.substr(index + 1,str.size() - index);
    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["userid"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();
    int len = send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len == -1){
         cerr<<"send groupchat msg error ->"<<endl;
    }
}
//quit command handler
void quit(int clientfd,string str){
    //退出之后 把用户id从在线表中删除 修改状态
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["userid"] = g_currentUser.getId();
     string buffer = js.dump();
    int len = send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len == -1){
         cerr<<"send quit msg error ->"<<endl;
    }
    else{
        isMainMenuRunning = false;
    }
}

//显示当前用户的基本信息
void showCurrentUserData(){
    
    cout<<"===================================================="<<endl;
    cout<<"current login user => id:"<<g_currentUser.getId()<<" name:"<<g_currentUser.getName()<<endl;
    cout<<"---------------------friend list---------------------"<<endl;
    if(!g_currentUserFriends.empty()){
        for(User &user : g_currentUserFriends){
            cout<<user.getId()<<" "<<user.getName()<<" "<<user.getState()<<endl;

        }
    }
    cout<<"---------------------group list---------------------"<<endl;
    if(!g_currentUserGroupList.empty()){
        for(Group &group :g_currentUserGroupList){
            cout<<group.getId()<<" "<<group.getName()<<" "<<group.getDesc()<<endl;
            for(GroupUser &user : group.getUsers()){
                cout<<user.getId()<<" "<<user.getName()<<" "<<user.getState()<<" "<<user.getRole()<<endl;
            }
        }
    }
    cout<<"===================================================="<<endl;
}

//获取系统时间（聊天信息需要添加时间信息）c++11封装好的方法
string getCurrentTime(){
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}

//注意测试 用户是否能收到多条离线消息
//注意客服端与服务端的json数据名称一致（userid和id），否则会出现[json.exception.type_error.302] type must be number, but is null
// [json.exception.type_error.302] type must be array, but is null ==> type must be string, but is null