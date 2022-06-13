#include "server_socket.h"
#include <string>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>

#include <netinet/tcp.h>
#include <netinet/in.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include "socket_defs.h"

struct sockaddr_in make_addr(uint16_t port)
{
	struct sockaddr_in sin{};
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);
	return sin;
}

struct sockaddr_in6 make_addr_v6(uint16_t port)
{
	struct sockaddr_in6 sin6{};
	sin6.sin6_flowinfo = 0;
	sin6.sin6_family = AF_INET6;
	sin6.sin6_addr = in6addr_any;
	sin6.sin6_port = htons(port);
	return sin6;
}

struct sockaddr_in make_addr_with_ip(const std::string& ip, uint16_t port)
{
	struct sockaddr_in sin{};
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);
	std::memcpy((char*)ip.c_str(), (char*)&sin.sin_addr, ip.length());
	return sin;
}

ServerSocket::ServerSocket()
	: _sockfd(INVALID_SOCKET), _blocking(false)
{
}

ServerSocket::ServerSocket(int sockfd)
	: _sockfd(sockfd), _blocking(false)
{
}

ServerSocket::~ServerSocket()
{
	close();
}

void ServerSocket::close()
{
	if (_sockfd != INVALID_SOCKET)
	{
		::close(_sockfd);
		_sockfd = INVALID_SOCKET;
	}
}

bool ServerSocket::shutdownReceive()
{
	int rc = ::shutdown(_sockfd, 0);
	if (rc != 0)
	{
		return false;
	}

	return true;
}

bool ServerSocket::shutdownSend()
{
	int rc = ::shutdown(_sockfd, 1);
	if (rc != 0)
	{
		return false;
	}

	return true;
}

bool ServerSocket::shutdown()
{
	int rc = ::shutdown(_sockfd, 2);
	if (rc != 0)
	{
		return false;
	}

	return true;
}

ssize_t ServerSocket::receiveBytes(void* buffer, int length, int flags)
{
	ssize_t rc;
	do
	{
		if (_sockfd == INVALID_SOCKET)
		{
			return -1;
		}
		rc = ::recv(_sockfd, reinterpret_cast<char*>(buffer), length, flags);
	} while (_blocking && rc < 0 && lastError() == EINTR);
	if (rc < 0)
	{
		int err = lastError();
		if (err == EAGAIN && !_blocking);
		else if (err == EAGAIN || err == ETIMEDOUT)
			return -2;
		else
			error(err);
	}
	return rc;
}

ssize_t ServerSocket::receiveBytes(SocketBufVec& buffers, int flags)
{
	ssize_t rc = 0;
	do
	{
		if (_sockfd == INVALID_SOCKET) throw std::runtime_error("invalid socket");
		rc = readv(_sockfd, &buffers[0], static_cast<int>(buffers.size()));
	} while (_blocking && rc < 0 && lastError() == EINTR);
	if (rc < 0)
	{
		int err = lastError();
		if (err == EAGAIN && !_blocking);
		else if (err == EAGAIN || err == ETIMEDOUT)
			throw std::runtime_error("receive data timeout");
		else
			error(err);
	}
	return rc;
}

ssize_t ServerSocket::receiveBytes(FIFOBuffer& fifoBuf)
{
	std::unique_lock<std::mutex> lock(fifoBuf.mutex());

	ssize_t ret = receiveBytes(fifoBuf.next(), (int)fifoBuf.available());
	if (ret > 0) fifoBuf.advance(ret);
	return ret;
}

ssize_t ServerSocket::sendBytes(const void* buffer, int length, int flags)
{
	ssize_t rc;
	do
	{
		if (_sockfd == INVALID_SOCKET)
		{
			return -1;
		}
		rc = ::send(_sockfd, reinterpret_cast<const char*>(buffer), length, flags);
	} while (_blocking && rc < 0 && lastError() == EINTR);
	if (rc < 0) error();
	return rc;
}

ssize_t ServerSocket::sendBytes(const SocketBufVec& buffers, int flags)
{
	ssize_t rc = 0;
	do
	{
		if (_sockfd == INVALID_SOCKET) throw std::runtime_error("invalid socket");
		rc = writev(_sockfd, &buffers[0], static_cast<int>(buffers.size()));
	} while (_blocking && rc < 0 && lastError() == EINTR);
	if (rc < 0) error();
	return rc;
}

ssize_t ServerSocket::sendBytes(FIFOBuffer& fifoBuf)
{
	std::unique_lock<std::mutex> lock(fifoBuf.mutex());

	ssize_t ret = sendBytes(fifoBuf.begin(), (int)fifoBuf.used());
	if (ret > 0) fifoBuf.drain(ret);
	return ret;
}

ServerSocket::Ptr ServerSocket::acceptConnection(sockaddr_in& clientAddr)
{
	if (_sockfd == INVALID_SOCKET)
	{
		return nullptr;
	}

	socklen_t len = sizeof(clientAddr);

	int sd;
	do
	{
		sd = ::accept(_sockfd, (struct sockaddr*)&clientAddr, &len);
	} while (sd == -1 && lastError() == EINTR);
	if (sd != -1)
	{
		return std::make_shared<ServerSocket>(sd);
//		return new ServerSocket(sd);
	}
	error();
	return nullptr;
}

void ServerSocket::connect(const std::string& ip, uint16_t port)
{
	if (_sockfd == INVALID_SOCKET)
	{
		init(AF_INET);
	}

	auto server_addr = make_addr_with_ip(ip, port);
	int rc;
	do
	{
		rc = ::connect(_sockfd, (sockaddr*)&server_addr, sizeof(server_addr));
	} while (rc != 0 && lastError() == EINTR);
	if (rc != 0)
	{
		int err = lastError();
		error(err, ip);
	}
}

void ServerSocket::connectNB(const std::string& ip, uint16_t port)
{
	if (_sockfd == INVALID_SOCKET)
	{
		init(AF_INET);
	}

	setBlocking(false);

	auto server_addr = make_addr_with_ip(ip, port);
	int rc = ::connect(_sockfd, (sockaddr*)&server_addr, sizeof(server_addr));
	if (rc != 0)
	{
		int err = lastError();
		if (err != EINPROGRESS && err != EWOULDBLOCK)
		{
			error(err, ip);
		}
	}
}

bool ServerSocket::bind(uint16_t port, bool reuseAddress)
{
	// 创建套接字
	if (_sockfd == INVALID_SOCKET)
	{
		init(AF_INET);
	}

	// 复用地址和端口
	if (reuseAddress)
	{
		setReuseAddress(true);
		setReusePort(true);
	}

	auto sin = make_addr(port);

	// 绑定地址
	int rc = ::bind(_sockfd, (struct sockaddr*)&sin, sizeof(sin));
	if (rc != 0)
	{
		return false;
	}
	return true;
}

void ServerSocket::bind(uint16_t port, bool reuseAddress, bool reusePort)
{
	if (_sockfd == INVALID_SOCKET)
	{
		init(AF_INET);
	}

	if (reuseAddress)
	{
		setReuseAddress(true);
	}
	if (reusePort)
	{
		setReusePort(true);
	}

	auto sin = make_addr(port);

	int rc = ::bind(_sockfd, (struct sockaddr*)&sin, sizeof(sin));
	if (rc != 0) error(port);
}

void ServerSocket::bind6(uint16_t port, bool reuseAddress, bool ipV6Only)
{
	bind6(port, reuseAddress, reuseAddress, ipV6Only);
}

void ServerSocket::bind6(uint16_t port, bool reuseAddress, bool reusePort, bool ipV6Only)
{
	if (_sockfd == INVALID_SOCKET)
	{
		init(AF_INET);
	}

	if (reuseAddress)
	{
		setReuseAddress(true);
	}
	if (reusePort)
	{
		setReusePort(true);
	}

#ifdef IPV6_V6ONLY
	setOption(IPPROTO_IPV6, IPV6_V6ONLY, ipV6Only ? 1 : 0);
#else
	if (ipV6Only) throw std::runtime_error("IPV6_V6ONLY not defined.");
#endif

	auto sin6 = make_addr_v6(port);

	int rc = ::bind(_sockfd, (struct sockaddr*)&sin6, sizeof(sin6));
	if (rc != 0) error(port);
}

bool ServerSocket::listen(int backlog)
{
	int rc = ::listen(_sockfd, backlog);
	if (rc != 0)
	{
		return false;
	}
	return true;
}

bool ServerSocket::ioctl(int request, int& arg)
{
	int rc = ::ioctl(_sockfd, request, &arg);
	if (rc != 0)
	{
		return false;
	}
	return true;
}

bool ServerSocket::ioctl(int request, void* arg)
{
	int rc = ::ioctl(_sockfd, request, arg);
	if (rc != 0)
	{
		return false;
	}
	return true;
}

int ServerSocket::fcntl(int request)
{
	int rc = ::fcntl(_sockfd, request);
	if (rc == -1) error();
	return rc;
}

int ServerSocket::fcntl(int request, long arg)
{
	int rc = ::fcntl(_sockfd, request, arg);
	if (rc == -1) error();
	return rc;
}

void ServerSocket::init(int af)
{
	initSocket(af, SOCK_STREAM, IPPROTO_TCP);
}

bool ServerSocket::initSocket(int af, int type, int proto)
{
	if (_sockfd != INVALID_SOCKET) return true;

	_sockfd = ::socket(af, type, proto);
	if (_sockfd == INVALID_SOCKET)
	{
		return false;
	}

	return true;
}

void ServerSocket::setSockfd(int aSocket)
{
	_sockfd = aSocket;
}

void ServerSocket::invalidate()
{
	_sockfd = INVALID_SOCKET;
}

int ServerSocket::lastError()
{
	return errno;
}

void ServerSocket::error()
{
	std::string empty;
	error(lastError(), empty);
}

void ServerSocket::error(const std::string& arg)
{
	error(lastError(), arg);
}

void ServerSocket::error(int code)
{
	std::string arg;
	error(code, arg);
}

void ServerSocket::error(int code, const std::string& arg)
{
	std::string msg = "ServerSocket error: ";
	msg += arg;
	msg += " (";
	msg += std::to_string(code);
	msg += ")";
	throw std::runtime_error(msg);
}

void ServerSocket::setOption(int level, int option, int value)
{
	setRawOption(level, option, &value, sizeof(value));
}

void ServerSocket::setOption(int level, int option, unsigned value)
{
	setRawOption(level, option, &value, sizeof(value));
}

void ServerSocket::setOption(int level, int option, unsigned char value)
{
	setRawOption(level, option, &value, sizeof(value));
}

void ServerSocket::setRawOption(int level, int option, const void* value, socklen_t length)
{
	if (_sockfd == INVALID_SOCKET)
	{
		return;
	}

	int rc = ::setsockopt(_sockfd, level, option, reinterpret_cast<const char*>(value), length);
	if (rc == -1) error();
}

void ServerSocket::getOption(int level, int option, int& value)
{
	socklen_t len = sizeof(value);
	getRawOption(level, option, &value, len);
}

void ServerSocket::getOption(int level, int option, unsigned& value)
{
	socklen_t len = sizeof(value);
	getRawOption(level, option, &value, len);
}

void ServerSocket::getOption(int level, int option, unsigned char& value)
{
	socklen_t len = sizeof(value);
	getRawOption(level, option, &value, len);
}

void ServerSocket::getRawOption(int level, int option, void* value, socklen_t& length)
{
	if (_sockfd == INVALID_SOCKET)
	{
		return;
	}

	int rc = ::getsockopt(_sockfd, level, option, reinterpret_cast<char*>(value), &length);
	if (rc == -1) error();
}

void ServerSocket::setLinger(bool on, int seconds)
{
	struct linger l{};
	l.l_onoff = on ? 1 : 0;
	l.l_linger = seconds;
	setRawOption(SOL_SOCKET, SO_LINGER, &l, sizeof(l));
}

void ServerSocket::getLinger(bool& on, int& seconds)
{
	struct linger l{};
	socklen_t len = sizeof(l);
	getRawOption(SOL_SOCKET, SO_LINGER, &l, len);
	on = l.l_onoff != 0;
	seconds = l.l_linger;
}

void ServerSocket::setNoDelay(bool flag)
{
	int value = flag ? 1 : 0;
	setOption(IPPROTO_TCP, TCP_NODELAY, value);
}

bool ServerSocket::getNoDelay()
{
	int value(0);
	getOption(IPPROTO_TCP, TCP_NODELAY, value);
	return value != 0;
}

void ServerSocket::setKeepAlive(bool flag)
{
	int value = flag ? 1 : 0;
	setOption(SOL_SOCKET, SO_KEEPALIVE, value);
}

bool ServerSocket::getKeepAlive()
{
	int value(0);
	getOption(SOL_SOCKET, SO_KEEPALIVE, value);
	return value != 0;
}

void ServerSocket::setReuseAddress(bool flag)
{
	int value = flag ? 1 : 0;
	setOption(SOL_SOCKET, SO_REUSEADDR, value);
}

bool ServerSocket::getReuseAddress()
{
	int value(0);
	getOption(SOL_SOCKET, SO_REUSEADDR, value);
	return value != 0;
}

void ServerSocket::setReusePort(bool flag)
{
#ifdef SO_REUSEPORT
	try
	{
		int value = flag ? 1 : 0;
		setOption(SOL_SOCKET, SO_REUSEPORT, value);
	}
	catch (...)
	{
		// ignore
	}
#endif
}

bool ServerSocket::getReusePort()
{
#ifdef SO_REUSEPORT
	int value(0);
	getOption(SOL_SOCKET, SO_REUSEPORT, value);
	return value != 0;
#else
	return false;
#endif
}

void ServerSocket::setOOBInline(bool flag)
{
	int value = flag ? 1 : 0;
	setOption(SOL_SOCKET, SO_OOBINLINE, value);
}

bool ServerSocket::getOOBInline()
{
	int value(0);
	getOption(SOL_SOCKET, SO_OOBINLINE, value);
	return value != 0;
}

void ServerSocket::setBroadcast(bool flag)
{
	int value = flag ? 1 : 0;
	setOption(SOL_SOCKET, SO_BROADCAST, value);
}

bool ServerSocket::getBroadcast()
{
	int value(0);
	getOption(SOL_SOCKET, SO_BROADCAST, value);
	return value != 0;
}

void ServerSocket::setBlocking(bool flag)
{
	int arg = fcntl(F_GETFL);
	long flags = arg & ~O_NONBLOCK;
	if (!flag) flags |= O_NONBLOCK;
	(void)fcntl(F_SETFL, flags);

	_blocking = flag;
}

bool ServerSocket::getBlocking() const
{
	return _blocking;
}

int ServerSocket::socketError()
{
	int result(0);
	getOption(SOL_SOCKET, SO_ERROR, result);
	return result;
}

void ServerSocket::setSendBufferSize(int size)
{
	setOption(SOL_SOCKET, SO_SNDBUF, size);
}

int ServerSocket::getSendBufferSize()
{
	int result;
	getOption(SOL_SOCKET, SO_SNDBUF, result);
	return result;
}

void ServerSocket::setReceiveBufferSize(int size)
{
	setOption(SOL_SOCKET, SO_RCVBUF, size);
}

int ServerSocket::getReceiveBufferSize()
{
	int result;
	getOption(SOL_SOCKET, SO_RCVBUF, result);
	return result;
}

int ServerSocket::available()
{
	int result = 0;
	ioctl(FIONREAD, result);
	return result;
}


