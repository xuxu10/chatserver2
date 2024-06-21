#ifndef GROUP_H
#define GROUP_H
#include <string>
#include <vector>
#include "groupuser.hpp"
using namespace std;
//allgroup表(属性：id groupname groupdesc)的ORM类 
class Group{
public:
    //构造函数 群组id=-1即该群不存在
    Group(int id = -1,string name = " ",string desc = " "){
        this->id = id;
        this->name = name;
        this->desc = desc;
    }
    //set
    void setId(int id){this->id = id;}
    void setName(string name){this->name = name;}
    void setDesc(string desc){this->desc = desc;}

    //get
    int getId(){return this->id;}
    string getName(){return this->name;}
    string getDesc(){return this->desc;}
    vector<GroupUser> &getUsers(){return this->users;}//获取该群的所有成员

private:
    int id;
    string name;
    string desc;//群组详细描述
    //群组成员类型的容器，存放一个群组的所有成员，只需要联合查询一次就可以得到群和成员对应的信息
    vector<GroupUser> users;
};
#endif