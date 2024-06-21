#ifndef DB_H
#define DB_H
#include <mysql/mysql.h>
#include <muduo/base/Logging.h>
#include <string>
using namespace std;


//数据库的操作类
class MySQL {
public:
    //初始化数据库连接
    MySQL();
    //释放数据库连接资源
    ~MySQL();
    //连接数据库
    bool connect();
    //更新操作
    bool update(string sql);
    //查询操作 MYSQL_RES:是一个结构体类型 返回查询结果
    MYSQL_RES *query(string sql);
    //获取当前连接
    MYSQL* getConn();
private:
    MYSQL *_conn;

};

#endif