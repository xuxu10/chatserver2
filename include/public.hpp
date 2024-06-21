#ifndef PUBLIC_H
#define PUBLIC_H

//server和client的公共文件
enum EnMsgType
{                   // 未被初始化的枚举值的值默认将比其前面的枚举值大1
    LOGIN_MSG = 1,  // 登录消息1
    REG_MSG,        // 注册消息2
    REG_MSG_ACK,    // 注册响应消息3
    LOGIN_MSG_ACK,  // 登录响应消息4
    ONE_CHAT_MSG,   // 一对一聊天消息5
    ADD_FRIEND_MSG, // 添加好友消息6
    CREATE_GROUP_MSG, // 创建群组消息7
    ADD_GROUP_MSG, // 加入群组消息8
    GROUP_CHAT_MSG, //群组聊天消息9
    LOGINOUT_MSG//正常退出消息10
};
#endif