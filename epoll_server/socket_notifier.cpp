#include "socket_notifier.h"
#include "server_socket.h"
#include "socket_reactor.h"
#include "socket_notification.h"

SocketNotifier::SocketNotifier(const ServerSocket& socket)
	: _socket(socket)
{
}

void SocketNotifier::addObserver(SocketReactor* pReactor, const AbstractObserver& observer)
{
	_nc.addObserver(observer);

	std::unique_lock<std::mutex> lock(_mutex);
	if (observer.accepts(pReactor->_pReadableNotification.get()))
		_events.insert(pReactor->_pReadableNotification.get());
	else if (observer.accepts(pReactor->_pWritableNotification.get()))
		_events.insert(pReactor->_pWritableNotification.get());
	else if (observer.accepts(pReactor->_pErrorNotification.get()))
		_events.insert(pReactor->_pErrorNotification.get());
	else if (observer.accepts(pReactor->_pTimeoutNotification.get()))
		_events.insert(pReactor->_pTimeoutNotification.get());
}

void SocketNotifier::removeObserver(SocketReactor* pReactor, const AbstractObserver& observer)
{
	_nc.removeObserver(observer);

	std::unique_lock<std::mutex> lock(_mutex);
	auto it = _events.end();
	if (observer.accepts(pReactor->_pReadableNotification.get()))
		it = _events.find(pReactor->_pReadableNotification.get());
	else if (observer.accepts(pReactor->_pWritableNotification.get()))
		it = _events.find(pReactor->_pWritableNotification.get());
	else if (observer.accepts(pReactor->_pErrorNotification.get()))
		it = _events.find(pReactor->_pErrorNotification.get());
	else if (observer.accepts(pReactor->_pTimeoutNotification.get()))
		it = _events.find(pReactor->_pTimeoutNotification.get());
	if (it != _events.end())
		_events.erase(it);
}

namespace
{
	ServerSocket nullSocket;
}

void SocketNotifier::dispatch(SocketNotification* pNotification)
{
	pNotification->setSocket(_socket);
//	pNotification->duplicate();
	try
	{
		_nc.postNotification(Notification::Ptr(pNotification));
	}
	catch (...)
	{
		pNotification->setSocket(nullSocket);
		throw;
	}
	pNotification->setSocket(nullSocket);
}
