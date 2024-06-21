#include "../../../include/server/model/friendmodel.hpp"
#include "../../../include/server/db/db.h"
#include <muduo/base/Logging.h>
//将好友关系id插入好友表
void FriendModel::insert(int userid,int friendid){
    char sql[1024] = {0};
    sprintf(sql,"insert into friend values(%d,%d)",userid,friendid);
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}

//返回用户好友列表 根据friendid在user表中查询
vector<User> FriendModel::query(int userid){
     //1.组件sql语句
    char sql[1024] = {0};
    //把结果输出到指定的字符串中
    //给定userid查询该用户的好友信息 friend表和user表联合(inner join)查询 结果为多个好友的： id name  state
    sprintf(sql,"select a.id,a.name,a.state from user a inner join friend b on b.friendid = a.id where b.userid = %d",userid);
    /*在frien表中userid和frienid联合确定一行数据，即1 2不等于 2 1，所以一对好友关系需要存两行数据
     */
    vector<User>vec;
    MySQL mysql;////MySQL 数据库的操作类
    if(mysql.connect()){
        MYSQL_RES *res = mysql.query(sql); 
        if(res!=nullptr){
            MYSQL_ROW row;

            while((row = mysql_fetch_row(res)) != nullptr){//每次获取一行，当查到的数据不为空时存入vector
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
                LOG_INFO<<"get friends!";

            }
            //释放mysql指针
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}