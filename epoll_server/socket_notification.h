#pragma once

#include "server_socket.h"
#include "notification.h"

class SocketReactor;

class SocketNotification : public Notification
{
public:
	explicit SocketNotification(SocketReactor* pReactor);
	~SocketNotification() override = default;

	SocketReactor& source() const;

	ServerSocket socket() const;

private:
	void setSocket(const ServerSocket& socket);

	SocketReactor* _pReactor = nullptr;
	ServerSocket _socket;

	friend class SocketNotifier;
};

class ReadableNotification : public SocketNotification
{
public:
	explicit ReadableNotification(SocketReactor* pReactor);
	~ReadableNotification() override = default;
};

class WritableNotification : public SocketNotification
{
public:
	explicit WritableNotification(SocketReactor* pReactor);
	~WritableNotification() override = default;
};

class ErrorNotification : public SocketNotification
{
public:
	explicit ErrorNotification(SocketReactor* pReactor);
	~ErrorNotification() override = default;
};

class TimeoutNotification : public SocketNotification
{
public:
	explicit TimeoutNotification(SocketReactor* pReactor);
	~TimeoutNotification() override = default;
};

class IdleNotification : public SocketNotification
{
public:
	explicit IdleNotification(SocketReactor* pReactor);
	~IdleNotification() override = default;
};

class ShutdownNotification : public SocketNotification
{
public:
	explicit ShutdownNotification(SocketReactor* pReactor);
	~ShutdownNotification() override = default;
};

//
// inlines
//
inline SocketReactor& SocketNotification::source() const
{
	return *_pReactor;
}

inline ServerSocket SocketNotification::socket() const
{
	return _socket;
}
