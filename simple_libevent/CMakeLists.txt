aux_source_directory(. DIR_SRCS)

include_directories(../)

# 增加生成可执行文件
add_executable(simple_libevent ${DIR_SRCS})

target_link_libraries(simple_libevent pthread event rt ${Boost_LIBRARIES})
