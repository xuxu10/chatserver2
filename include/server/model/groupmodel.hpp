#ifndef GROUPMODEL_H
#define GROUPMODEL_H
#include "group.hpp"
#include <vector>
//维护群组信息的操作接口方法
class GroupModel{
public:
    //创建群组
    bool createGroup(Group &group);
    //加入群组
    void addGroup(int userid,int groupid,string role);
    //查询用户所在群组的信息:一个用户可能在多个群，拥有不同的身份，所以用vector存储多个群组的信息
    vector<Group> queryGroups(int userid);
    //发送群消息：userid和groupid确定谁在哪个群发消息，根据其他成员id发送消息
    //查询群组里除了该用户以外，其他成员的id
    vector<int> queryGroupUsers(int userid,int groupid);//通过_userConnMap里保存的对应id的连接直接转发
    

};
#endif