aux_source_directory(. DIR_SRCS)

include_directories(../)

# 增加生成可执行文件
add_executable(simple_epoll ${DIR_SRCS} main.cpp stdinc.h event.h event.cpp server.h server.cpp)

target_link_libraries(simple_epoll pthread ${Boost_LIBRARIES})
