#include "../../../include/server/model/groupmodel.hpp"
#include "../../../include/server/db/db.h"

//创建群组
bool GroupModel::createGroup(Group &group){
    char sql[1024] = {0};
    //id是auto_increment自动生成的
    sprintf(sql,"insert into allgroup(groupname,groupdesc) values('%s','%s')",group.getName().c_str(),group.getDesc().c_str());
    MySQL mysql;
    if(mysql.connect()){
        if(mysql.update(sql)){
            //如果生成一条记录
            //记录群组id
            group.setId(mysql_insert_id(mysql.getConn()));
            return true;
        }
    }
    return false;//连接数据库失败
}
//加入群组 将userid groupid 插入groupuser表
void GroupModel::addGroup(int userid,int groupid,string role){
    char sql[1024] = {0};
    
    sprintf(sql,"insert into groupuser values(%d,%d,'%s')",groupid,userid,role.c_str());
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
        
    }
    
}
//查询用户所在群组的信息:一个用户可能在多个群，拥有不同的身份，所以用vector存储多个群组的信息
vector<Group> GroupModel::queryGroups(int userid){
    
    char sql[1024] = {0};
    // allgroup表和groupuser表联合(inner join)查询 结果为该用户所在群组信息： id groupname groupdesc (由于返回的是group类型grouprole不用查询）
    sprintf(sql,"select a.id,a.groupname,a.groupdesc from allgroup a inner join \
                 groupuser b on b.groupid = a.id where b.userid = %d",userid);
    
    vector<Group>groupvec;
    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES *res = mysql.query(sql); 
        if(res!=nullptr){
            MYSQL_ROW row;

            while((row = mysql_fetch_row(res)) != nullptr){//每次获取一行，当查到的数据不为空时存入vector
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupvec.push_back(group);
                //LOG_INFO<<"get groups!";
                

            }
            //释放mysql指针
            mysql_free_result(res);
            
        }
    }
    //查询该组的所有成员信息
    for(Group & group : groupvec){
        //根据群id 查
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from user a inner join \
                 groupuser b on b.userid = a.id where b.groupid = %d",group.getId());
   
            MYSQL_RES *res = mysql.query(sql);
            if (res != nullptr)
            {
                MYSQL_ROW row;

                while ((row = mysql_fetch_row(res)) != nullptr)
                { // 每次获取一行，当查到的数据不为空时存入vector
                    GroupUser user;
                    user.setId(atoi(row[0]));
                    user.setName(row[1]);
                    user.setState(row[2]);
                    user.setRole(row[3]);
                    group.getUsers().push_back(user);
                    
                }
                // 释放mysql指针
                mysql_free_result(res);
            }
    }

    return groupvec;
}
//发送群消息：userid和groupid确定谁在哪个群发消息，根据其他成员id发送消息
//查询群组里除了该用户以外，其他成员的id
vector<int> GroupModel::queryGroupUsers(int userid,int groupid){
    char sql[1024] = {0};
    
    sprintf(sql,"select userid from groupuser where groupid = %d and userid != %d",groupid,userid);
    
    vector<int> groupUserVec;
    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES *res = mysql.query(sql); 
        if(res!=nullptr){
            MYSQL_ROW row;

            while((row = mysql_fetch_row(res)) != nullptr){//查询到多条信息
                groupUserVec.push_back(atoi(row[0]));
            }
            //释放mysql指针
            mysql_free_result(res);
            
        }
    }
    return groupUserVec;
}