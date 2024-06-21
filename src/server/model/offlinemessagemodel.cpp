#include "../../../include/server/model/offlinemessagemodel.hpp"
#include "../../../include/server/db/db.h"
//根据id和msg存储离线消息
void OfflineMsgModel::insert(int userid,string msg){
    char sql[1024] = {0};
    sprintf(sql,"insert into offlineMessage values(%d,'%s')",userid,msg.c_str());
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }


}

//删除用户离线消息，防止重复发送
void OfflineMsgModel::remove(int userid){
    char sql[1024] = {0};
    sprintf(sql,"delete from offlineMessage where userid =%d ",userid);
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }

}

//查询用户的离线消息
vector<string>OfflineMsgModel::query(int userid){
    //1.组件sql语句
    char sql[1024] = {0};
    //把结果输出到指定的字符串中
    sprintf(sql,"select message from offlineMessage where userid = %d",userid);
    vector<string>vec;
    MySQL mysql;////MySQL 数据库的操作类
    if(mysql.connect()){
        MYSQL_RES *res = mysql.query(sql); 
        if(res!=nullptr){
            MYSQL_ROW row;

            while((row = mysql_fetch_row(res)) != nullptr){//每次获取一行，当查到的数据不为空时存入vector
                vec.push_back(row[0]);

            }
            //释放mysql指针
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}