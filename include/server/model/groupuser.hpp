#ifndef GROUPUSER_H
#define GROUPUSER_H
#include "user.hpp"
//群组成员表类，是继承user的，比user多一个属性：grouprole
class GroupUser : public User{
public:
    void setRole(string role){this->role = role;}
    string getRole(){return this->role;}
private:
    string role;
};
#endif