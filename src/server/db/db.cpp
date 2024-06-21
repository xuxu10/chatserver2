#include "../../../include/server/db/db.h"
#include <muduo/base/Logging.h>

//数据库配置信息
static string server = "127.0.0.1";
static string user = "root";
static string password = "123456";
static string dbname = "chat";

MySQL::MySQL(){
    //mysql_init初始化MYSQL变量
    _conn = mysql_init(nullptr);
}
//释放数据库连接资源
MySQL:: ~MySQL(){
    if(_conn != nullptr){
        //mysql_close函数关闭数据库连接
        mysql_close(_conn);
    }
}
//连接数据库
bool MySQL::connect(){
    //mysql_real_connect()函数连接Mysql数据库
    MYSQL *p = mysql_real_connect(_conn,server.c_str(),user.c_str(),password.c_str(),
                                    dbname.c_str(),3306,nullptr,0);
    if(p != nullptr){
        //c和c++代码默认的编码字符是ASCII，如果不设置，从MySQL拉下来的中文显示？
        mysql_query(_conn,"set names gbk");
        LOG_INFO<<"connect mysql success!";
    }
    else{
         LOG_INFO<<"connect mysql fail!";
    }
    return p;
}

//更新操作
bool MySQL::update(string sql){
    if(mysql_query(_conn,sql.c_str())){
        LOG_INFO << __FILE__ <<":"<< __LINE__ <<":"<<sql<<"更新失败";
        return false;
    }
    return true;
}
//查询操作 MYSQL_RES:是一个结构体类型 返回查询结果
MYSQL_RES * MySQL::query(string sql){
    //mysql_query()查询数据库某表内容
    if(mysql_query(_conn,sql.c_str())){
        LOG_INFO << __FILE__ <<":"<< __LINE__ <<":"<<sql<<"查询失败";
        return nullptr;
    }
    //mysql_store_result()是把查询全部做完，然后一次性将查询结果返回
    //mysql_use_result()是逐条进行查询，逐条将结果返回给客户端 
    return mysql_use_result(_conn);
}

//获取当前连接
    MYSQL*  MySQL::getConn(){
        return this->_conn;
    }