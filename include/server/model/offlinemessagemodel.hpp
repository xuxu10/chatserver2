#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H
#include <string>
#include <vector>
using namespace std;
//处理离线表的操作
class OfflineMsgModel{
public:
    //根据id和msg存储离线消息
    void insert(int userid,string msg);

    //删除用户离线消息，防止重复发送
    void remove(int userid);

    //查询用户的离线消息
    vector<string>query(int userid);
};

#endif