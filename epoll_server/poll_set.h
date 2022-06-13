#pragma once

#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <map>
#include <set>
#include <mutex>
#include <sys/socket.h>
#include <sys/epoll.h>

#include "server_socket.h"

class PollSet : public boost::noncopyable
{
public:
	enum Mode
	{
		POLL_READ = 0x01,
		POLL_WRITE = 0x02,
		POLL_ERROR = 0x04
	};

	using SocketModeMap = std::map<ServerSocket, int>;

	PollSet();
	~PollSet();

	void add(const ServerSocket& socket, int mode);

	void remove(const ServerSocket& socket);

	void update(const ServerSocket& socket, int mode);

	bool has(const ServerSocket& socket) const;

	bool empty() const;

	void clear();

	SocketModeMap poll(std::chrono::system_clock::duration timeout);

private:
	void create_epollfd();

	void destroy_epollfd();

private:
	mutable std::mutex _mutex;

	int _epollfd;

	using ServerSocketMap = std::map<void*, ServerSocket>;
	using ServerSocketSet = std::set<ServerSocket>;
	using EpollEventArray = std::vector<struct epoll_event>;

	ServerSocketMap _socketMap;
	EpollEventArray _events;
};
