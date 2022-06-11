aux_source_directory(. DIR_SRCS)

include_directories(../)

# 增加生成可执行文件
add_executable(raw_echo_client ${DIR_SRCS} main.cpp)

target_link_libraries(raw_echo_client pthread ${Boost_LIBRARIES})
