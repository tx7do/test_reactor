aux_source_directory(. DIR_SRCS)

include_directories(../)

# 增加生成可执行文件
add_executable(epoll_server ${DIR_SRCS} main.cpp socket_defs.h poll_set.cpp poll_set.h server_socket.cpp server_socket.h socket_reactor.h socket_reactor.cpp socket_connector.h socket_acceptor.h server_application.h server_application.cpp echo_server.h echo_server.cpp event.h event.cpp buffer.h fifo_buffer.h observer.h notification.h notification.cpp notification_center.h notification_center.cpp socket_notification.h socket_notification.cpp socket_notifier.h socket_notifier.cpp)

target_link_libraries(epoll_server pthread rt ${Boost_LIBRARIES})
