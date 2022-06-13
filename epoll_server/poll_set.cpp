
#include "poll_set.h"
#include "server_socket.h"
#include "socket_defs.h"

PollSet::PollSet()
	: _epollfd(-1), _events(1024)
{
	create_epollfd();
}

PollSet::~PollSet()
{
	destroy_epollfd();
}

void PollSet::add(const ServerSocket& socket, int mode)
{
	std::lock_guard<std::mutex> guard(_mutex);

	int fd = socket.sockfd();

	struct epoll_event ev{ .events =  0 };
	if (mode & PollSet::POLL_READ)
	{
		ev.events |= EPOLLIN;
	}
	if (mode & PollSet::POLL_WRITE)
	{
		ev.events |= EPOLLOUT;
	}
	if (mode & PollSet::POLL_ERROR)
	{
		ev.events |= EPOLLERR;
	}
	ev.data.ptr = (void*)&socket;

	int err = epoll_ctl(_epollfd, EPOLL_CTL_ADD, fd, &ev);
	if (err)
	{
		if (errno == EEXIST) update(socket, mode);
		else ServerSocket::error();
	}

	if (_socketMap.find(socket) == _socketMap.end())
	{
		_socketMap[socket] = socket;
	}
}

void PollSet::remove(const ServerSocket& socket)
{
	std::lock_guard<std::mutex> guard(_mutex);

	auto fd = socket.sockfd();
	struct epoll_event ev{ 0, { nullptr }};
	int err = epoll_ctl(_epollfd, EPOLL_CTL_DEL, fd, &ev);
	if (err) ServerSocket::error();

	_socketMap.erase(socket);
}

void PollSet::update(const ServerSocket& socket, int mode)
{
	int fd = socket.sockfd();

	struct epoll_event ev{ .events =  0 };
	ev.events = 0;
	if (mode & PollSet::POLL_READ)
	{
		ev.events |= EPOLLIN;
	}
	if (mode & PollSet::POLL_WRITE)
	{
		ev.events |= EPOLLOUT;
	}
	if (mode & PollSet::POLL_ERROR)
	{
		ev.events |= EPOLLERR;

	}
	ev.data.ptr = (void*)&socket;

	int err = epoll_ctl(_epollfd, EPOLL_CTL_MOD, fd, &ev);
	if (err)
	{
		ServerSocket::error();
	}
}

bool PollSet::has(const ServerSocket& socket) const
{
	std::lock_guard<std::mutex> guard(_mutex);
	return (_socketMap.find(socket) != _socketMap.end());
}

bool PollSet::empty() const
{
	std::lock_guard<std::mutex> guard(_mutex);
	return _socketMap.empty();
}

void PollSet::clear()
{
	std::lock_guard<std::mutex> guard(_mutex);

	_socketMap.clear();

	::close(_epollfd);
	_epollfd = epoll_create(1);
	if (_epollfd < 0)
	{
		ServerSocket::error();
	}
}

PollSet::SocketModeMap PollSet::poll(std::chrono::system_clock::duration timeout)
{
	PollSet::SocketModeMap result;

	{
		std::lock_guard<std::mutex> guard(_mutex);
		if (_socketMap.empty()) return result;
	}

	int rc;
	do
	{
		rc = epoll_wait(_epollfd, &_events[0], _events.size(), 0);
		if (rc < 0 && ServerSocket::lastError() == EINTR)
		{
			continue;
		}
	} while (rc < 0 && ServerSocket::lastError() == EINTR);
	if (rc < 0) ServerSocket::error();

	std::lock_guard<std::mutex> guard(_mutex);

	for (int i = 0; i < rc; i++)
	{
		auto it = _socketMap.find(_events[i].data.ptr);
		if (it != _socketMap.end())
		{
			if (_events[i].events & EPOLLIN)
				result[it->second] |= PollSet::POLL_READ;
			if (_events[i].events & EPOLLOUT)
				result[it->second] |= PollSet::POLL_WRITE;
			if (_events[i].events & EPOLLERR)
				result[it->second] |= PollSet::POLL_ERROR;
		}
	}

	return result;
}

void PollSet::create_epollfd()
{
	_epollfd = epoll_create(1);
	if (_epollfd < 0)
	{
		ServerSocket::error();
	}
}

void PollSet::destroy_epollfd()
{
	if (_epollfd >= 0)
	{
		::close(_epollfd);
		_epollfd = -1;
	}
}
