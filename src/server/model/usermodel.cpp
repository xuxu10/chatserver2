#include "../../../include/server/model/usermodel.hpp"
#include "../../../include/server/db/db.h"
#include <iostream>
using namespace std;
//User表的增加方法
bool UserModel::insert(User &user){//&user 引用类型 最后的id直接添加到user中
    //1.组装sql语句
    char sql[1024] = {0};
    //sprintf()把结果输出到指定的字符串中
    sprintf(sql,"insert into user(name,password,state) values('%s','%s','%s')",
            user.getName().c_str(),user.getPwd().c_str(),user.getState().c_str());
            //c_str()将string转成一个const char*指针
    MySQL mysql;
    if(mysql.connect()){//连接数据库

        if(mysql.update(sql)){//更新操作

            //获取插入成功的用户数据生成的id
            user.setId(mysql_insert_id(mysql.getConn()));
            return true;
        }
    }
    return false;
}
User UserModel::query(int id){
    //1.组件sql语句
    char sql[1024] = {0};
    //把结果输出到指定的字符串中
    sprintf(sql,"select * from user where id = %d",id);
    MySQL mysql;////MySQL 数据库的操作类
    if(mysql.connect()){
        MYSQL_RES *res = mysql.query(sql); 
        if(res!=nullptr){
            MYSQL_ROW row = mysql_fetch_row(res);//获取一行
            if(row != nullptr){//查到的数据不为空
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                //释放mysql指针
                mysql_free_result(res);
                return user;

            }
        }
    }
    return User();//找不到返回 默认构造一个user它的id为-1表示不存在
}
bool UserModel::updateState(User user){
     //1.组件sql语句
    char sql[1024] = {0};
    //把结果输出到指定的字符串中
    sprintf(sql,"update user set state = '%s' where id = %d",user.getState().c_str(),user.getId());
    MySQL mysql;
    if(mysql.connect()){//连接数据库

        if(mysql.update(sql)){//更新操作

            //更新成功
            return true;
        }
    }
    return false;
}

//重置用户状态
void UserModel::resetState(){
     //1.组件sql语句
    char sql[1024] = {0};
    //把结果输出到指定的字符串中
    sprintf(sql,"update user set state = 'offline' where state = 'online'");
    MySQL mysql;
    if(mysql.connect()){//连接数据库

        mysql.update(sql);
    }
    
}