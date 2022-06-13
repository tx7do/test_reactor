aux_source_directory(. DIR_SRCS)

include_directories(../)

# 增加生成可执行文件
add_executable(libevent_server ${DIR_SRCS} main.cpp)

target_link_libraries(libevent_server pthread event ${Boost_LIBRARIES})
