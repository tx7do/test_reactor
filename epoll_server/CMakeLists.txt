aux_source_directory(. DIR_SRCS)

include_directories(../)

# 增加生成可执行文件
add_executable(epoll_server ${DIR_SRCS})

target_link_libraries(epoll_server pthread rt ${Boost_LIBRARIES})
