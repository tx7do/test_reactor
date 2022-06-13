#pragma once

#include <vector>
#include <net/if.h>

#define INVALID_SOCKET  -1
#define socket_t        int
#define socklen_t       socklen_t
#define fcntl_request_t int
#define closesocket(s)  ::close(s)


typedef iovec SocketBuf;
typedef std::vector<SocketBuf> SocketBufVec;
