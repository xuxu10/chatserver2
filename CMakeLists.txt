#cmake配置编译环境
#最低版本号
cmake_minimum_required(VERSION 3.0)
#项目名称 聊天服务器
project(CHAT)

#配置编译选项 使用 -g 标志生成调试信息
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

#配置最终可执行文件输出的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#头文件的搜索路径
include_directories(${PROJECT_BINARY_DIR}/include)
include_directories(${PROJECT_BINARY_DIR}/include/server)
include_directories(${PROJECT_BINARY_DIR}/include/server/db)
include_directories(${PROJECT_BINARY_DIR}/include/server/model)
include_directories(${PROJECT_BINARY_DIR}/include/server/redis)
include_directories(${PROJECT_BINARY_DIR}/thirdparty)


#加载子目录 src  源文件
add_subdirectory(src)