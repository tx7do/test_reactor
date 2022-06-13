
#pragma once

#include <string>
#include <unistd.h>
#include <memory>

#include "fifo_buffer.h"
#include "socket_defs.h"

class ServerSocket
{
public:
	ServerSocket();
	explicit ServerSocket(int sockfd);
	virtual ~ServerSocket();

	using ServerSocketList = std::vector<ServerSocket>;
	using Ptr = std::shared_ptr<ServerSocket>;

	bool operator==(const ServerSocket& socket) const;
	bool operator!=(const ServerSocket& socket) const;
	bool operator<(const ServerSocket& socket) const;
	bool operator<=(const ServerSocket& socket) const;
	bool operator>(const ServerSocket& socket) const;
	bool operator>=(const ServerSocket& socket) const;

public:
	int sockfd() const
	{
		return _sockfd;
	}

	bool initialized() const
	{
		return _sockfd != -1;
	}

	int available();

public:
	bool ioctl(int request, int& arg);
	bool ioctl(int request, void* arg);

	int fcntl(int request);
	int fcntl(int request, long arg);

	bool shutdownReceive();
	bool shutdownSend();
	bool shutdown();

	void close();

	void connect(const std::string& ip, uint16_t port);
	void connectNB(const std::string& ip, uint16_t port);

	bool bind(uint16_t port, bool reuseAddress = false);
	void bind(uint16_t port, bool reuseAddress, bool reusePort);
	void bind6(uint16_t port, bool reuseAddress = false, bool ipV6Only = false);
	void bind6(uint16_t port, bool reuseAddress, bool reusePort, bool ipV6Only);

	bool listen(int backlog = 64);

	ServerSocket::Ptr acceptConnection(struct sockaddr_in& clientAddr);

	ssize_t sendBytes(const void* buffer, int length, int flags = 0);
	ssize_t sendBytes(const SocketBufVec& buffers, int flags);
	ssize_t sendBytes(FIFOBuffer& buffer);

	ssize_t receiveBytes(void* buffer, int length, int flags = 0);
	ssize_t receiveBytes(SocketBufVec& buffers, int flags);
	ssize_t receiveBytes(FIFOBuffer& buffer);

public:
	void setOption(int level, int option, int value);
	void setOption(int level, int option, unsigned value);
	void setOption(int level, int option, unsigned char value);
	void setRawOption(int level, int option, const void* value, socklen_t length);

	void getOption(int level, int option, int& value);
	void getOption(int level, int option, unsigned& value);
	void getOption(int level, int option, unsigned char& value);
	virtual void getRawOption(int level, int option, void* value, socklen_t& length);

	void setLinger(bool on, int seconds);
	void getLinger(bool& on, int& seconds);

	void setNoDelay(bool flag);
	bool getNoDelay();

	void setKeepAlive(bool flag);
	bool getKeepAlive();

	void setReuseAddress(bool flag);
	bool getReuseAddress();

	void setReusePort(bool flag);
	bool getReusePort();

	void setOOBInline(bool flag);
	bool getOOBInline();

	void setBroadcast(bool flag);
	bool getBroadcast();

	void setBlocking(bool flag);
	bool getBlocking() const;

	void setSendBufferSize(int size);
	int getSendBufferSize();

	void setReceiveBufferSize(int size);
	int getReceiveBufferSize();

	int socketError();

public:
	static int lastError();

	static void error();

	static void error(const std::string& arg);

	static void error(int code);

	static void error(int code, const std::string& arg);

protected:
	void init(int af);

	bool initSocket(int af, int type, int proto = 0);

	void invalidate();

	void setSockfd(int aSocket);

protected:
	int _sockfd = -1;

	bool _blocking = false;
	bool _isBrokenTimeout = false;
};

inline bool ServerSocket::operator==(const ServerSocket& socket) const
{
	return _sockfd == socket._sockfd;
}

inline bool ServerSocket::operator!=(const ServerSocket& socket) const
{
	return _sockfd != socket._sockfd;
}

inline bool ServerSocket::operator<(const ServerSocket& socket) const
{
	return _sockfd < socket._sockfd;
}

inline bool ServerSocket::operator<=(const ServerSocket& socket) const
{
	return _sockfd <= socket._sockfd;
}

inline bool ServerSocket::operator>(const ServerSocket& socket) const
{
	return _sockfd > socket._sockfd;
}

inline bool ServerSocket::operator>=(const ServerSocket& socket) const
{
	return _sockfd >= socket._sockfd;
}
