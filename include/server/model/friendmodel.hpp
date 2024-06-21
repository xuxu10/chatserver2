#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H
#include <vector>
#include "user.hpp"
//friend表的数据操作类
class FriendModel{
public:
    //将好友关系id插入好友表
    void insert(int userid,int friendid);

    //返回用户好友列表 根据friendid在user表中查询
    vector<User> query(int userid);
};
#endif