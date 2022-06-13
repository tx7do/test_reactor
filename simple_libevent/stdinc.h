#pragma once

#include <cstring>
#include <cerrno>
#include <cstdio>
#include <csignal>

#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
#include <sys/socket.h>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>


static void handle_accept(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr* sa, int socklen, void* user_arg);
static void handle_recv(struct bufferevent*, void*);
static void handle_send(struct bufferevent*, void*);
static void handle_event(struct bufferevent*, short, void*);
static void handle_signal(evutil_socket_t, short, void*);
