#include "socket_notification.h"

SocketNotification::SocketNotification(SocketReactor* pReactor)
	: _pReactor(pReactor)
{
}

void SocketNotification::setSocket(const ServerSocket& socket)
{
	_socket = socket;
}

ReadableNotification::ReadableNotification(SocketReactor* pReactor)
	: SocketNotification(pReactor)
{
}

WritableNotification::WritableNotification(SocketReactor* pReactor)
	: SocketNotification(pReactor)
{
}

ErrorNotification::ErrorNotification(SocketReactor* pReactor)
	: SocketNotification(pReactor)
{
}

TimeoutNotification::TimeoutNotification(SocketReactor* pReactor)
	: SocketNotification(pReactor)
{
}

IdleNotification::IdleNotification(SocketReactor* pReactor)
	: SocketNotification(pReactor)
{
}

ShutdownNotification::ShutdownNotification(SocketReactor* pReactor)
	: SocketNotification(pReactor)
{
}
