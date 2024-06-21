#include "../../../include/server/redis/redis.hpp"
#include <iostream>
#include <thread>
//同步的 基于发布-订阅的redis消息队列  对应相同功能代码是同用的
Redis::Redis(): _publish_context(nullptr),_subscribe_context(nullptr){
   //把两个redisContext指针置为空
}
Redis::~Redis(){
    //如果指针不为空就释放
    if(_publish_context != nullptr){
        redisFree(_publish_context);
    }
    if(_subscribe_context != nullptr){
        redisFree(_subscribe_context);
    }
}
//连接redis服务器
bool Redis::connect(){
    //负责publish发布消息的上下文连接
    _publish_context = redisConnect("127.0.0.1",6379);//6379redis端口号
    if(nullptr == _publish_context){
        cerr<<"connect redis failed"<<endl;
        return false;
    }
    // //负责subscribe发布消息的上下文连接
    _subscribe_context = redisConnect("127.0.0.1",6379);//6379redis端口号
    if(nullptr == _subscribe_context){
        cerr<<"connect redis failed"<<endl;
        return false;
    }
    //在单独的鲜橙汁，监听通道上的时间，有消息就给业务层上报
    thread t([&](){
        observer_channel_message();
    });
    t.detach();
    
    cout<<"connect redis-server success"<<endl;
    return true;

}
//向redis指定的通道channel发布消息
bool Redis::publish(int channel,string message){
    //在publish使用redisCommand是因为，它执行publish命令后马上回复不会发生阻塞
    redisReply *reply = (redisReply *)redisCommand(_publish_context,"PUBLISH %d %s",channel,message.c_str());
    if(nullptr == reply){
        cerr<<"publish command failed"<<endl;
        return false;
    }
    freeReplyObject(reply);
    return true;


}
//向redis指定的通道channel订阅消息
bool Redis::subscribe(int channel){
    //在subscribe不使用redisCommand是因为，它执行subscribe命令后不会马上得到回复，就阻塞了
    /**
     * 解决办法：只做订阅通道，不接受通道消息
     * 通道消息的接收专门在observer_channel_message函数中的独立线程中进行
     * subscrib只负责发送命令，不阻塞接收redis server响应消息，否则和notifyMsg线程抢占响应资源
     * 
     * redisCommand调用：1.redisAppendCommand   2.redisBufferWrite
     * 3.redisGetReply 阻塞等待redis server响应消息
     */
    //redisAppendCommand 把命令写入本地发送缓冲区
    if(REDIS_ERR == redisAppendCommand(this->_subscribe_context,"SUBSCRIBE %d",channel)){
        cerr<<"subscribr command failed"<<endl;
        return false;
    }
    int done = 0;
    while (!done)
    {
        //redisBufferWrite 把本地缓冲区的命令通过网络发送出去
        if(REDIS_ERR == redisBufferWrite(this->_subscribe_context,&done)){
            cerr<<"subscribr command failed"<<endl;
            return false;
        }
    }
    return true;
    
}
//向redis指定的通道channel取消订阅消息
bool Redis::unsubscribe(int channel){
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel))
    {
        cerr << "unsubscribe command failed!" << endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "unsubscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}

//在独立线程中接收订阅通道中的消息
void Redis::observer_channel_message(){
    redisReply *reply = nullptr;
    while (REDIS_OK == redisGetReply(this->_subscribe_context,(void **)&reply))
    {
        //订阅收到的消息element是一个带三元素的数组：element[0]="message" element[1] = 通道号 element[2]=传输的消息
        if(reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr){
           //给业务层上报通道发生的消息
            _notify_message_handler(atoi(reply->element[1]->str),reply->element[2]->str);
        }
        freeReplyObject(reply);
        
    }
    cerr<<"================observer_channel_message quit======================"<<endl;
    
}

//初始化向业务层上报通道消息的回调对象
void Redis::init_notify_handler(function<void(int,string)> fn){
    this->_notify_message_handler = fn;

}