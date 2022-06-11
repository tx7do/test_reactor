#pragma once

#include <cstdio>
#include <unistd.h>
#include <cstdint>

#define SERVER_PORT   8080

static short g_port = SERVER_PORT;


int init_listen_socket(short port);

void check_online();

void handle_accept(int fd, uint32_t events, void* arg);
void handle_recv(int fd, uint32_t events, void* arg);
void handle_send(int fd, uint32_t events, void* arg);

int start_server();
