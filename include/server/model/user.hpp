#ifndef USER_H
#define USER_H

#include <string>
using namespace std;
class User{
public:
    User(int id=-1,string name="",string pwd = "",string state="offline"){
        this->id = id;
        this->name = name;
        this->pwd = pwd;
        this->state = state;

    }
    void setId(int id){
        this->id = id;
    }
    void setName(string name){
        this->name = name;
    }
     void setPwd(string pwd){
        this->pwd = pwd;
    }
     void setState(string state){
        this->state = state;
    }
    int getId(){return this->id;}
    string getName(){return this->name;}
    string getPwd(){return this->pwd;}
    string getState(){return this->state;}
protected://可以继承给群组成员
    int id;
    string name;
    string pwd;
    string state;
};

#endif