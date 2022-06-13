#pragma once

#include <netinet/in.h>
#include "server_socket.h"
#include "socket_reactor.h"
#include "observer.h"
#include "socket_notification.h"

template<class ServiceHandler>
class SocketAcceptor
{
public:
	using Observer = Observer<SocketAcceptor, ReadableNotification>;

	explicit SocketAcceptor(ServerSocket& socket)
		: _socket(socket), _pReactor(nullptr)
	{
	}

	explicit SocketAcceptor(ServerSocket& socket, SocketReactor& reactor)
		: _socket(socket), _pReactor(&reactor)
	{
		_pReactor->addEventHandler(_socket, Observer(*this, &SocketAcceptor::onAccept));
	}

	virtual ~SocketAcceptor()
	{
		try
		{
			if (_pReactor)
			{
				_pReactor->removeEventHandler(_socket, Observer(*this, &SocketAcceptor::onAccept));
			}
		}
		catch (...)
		{
		}
	}

	void setReactor(SocketReactor& reactor)
	{
		registerAcceptor(reactor);
	}

	virtual void registerAcceptor(SocketReactor& reactor)
	{
		_pReactor = &reactor;
		if (!_pReactor->hasEventHandler(_socket, Observer(*this, &SocketAcceptor::onAccept)))
		{
			_pReactor->addEventHandler(_socket, Observer(*this, &SocketAcceptor::onAccept));
		}
	}

	virtual void unregisterAcceptor()
	{
		if (_pReactor)
		{
			_pReactor->removeEventHandler(_socket, Observer(*this, &SocketAcceptor::onAccept));
		}
	}

	void onAccept(ReadableNotification* pNotification)
	{
		struct sockaddr_in clientAddr{};
		auto sock = _socket.acceptConnection(clientAddr);
		_pReactor->wakeUp();
		createServiceHandler(reinterpret_cast<ServerSocket&>(*sock.get()));
	}

protected:
	virtual ServiceHandler* createServiceHandler(ServerSocket& socket)
	{
		return new ServiceHandler(socket, *_pReactor);
	}

	SocketReactor* reactor()
	{
		return _pReactor;
	}

	ServerSocket& socket()
	{
		return _socket;
	}

private:
	ServerSocket _socket;
	SocketReactor* _pReactor;
};
