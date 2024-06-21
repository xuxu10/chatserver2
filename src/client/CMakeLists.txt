#定义SRC_LIST变量 存储当前路径下所有的源文件
aux_source_directory(. SRC_LIST)


#指定生成可执行文件ChatServer
add_executable(ChatClient ${SRC_LIST} )
#ChatClient文件链接时需要依赖的库文件 聊天的客户端有读写两个线程
target_link_libraries(ChatClient  pthread)