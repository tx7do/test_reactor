#pragma once

#include <netinet/in.h>
#include "server_socket.h"
#include "socket_reactor.h"
#include "observer.h"
#include "socket_notification.h"

template<class ServiceHandler>
class SocketConnector
{
public:
	SocketConnector() = delete;
	SocketConnector(const SocketConnector&) = delete;
	SocketConnector& operator=(const SocketConnector&) = delete;

	explicit SocketConnector(const std::string& ip, uint16_t port)
		: _pReactor(nullptr)
	{
		_socket.connectNB(ip, port);
	}

	SocketConnector(const std::string& ip, uint16_t port, SocketReactor& reactor, bool doRegister = true)
		: _pReactor(nullptr)
	{
		_socket.connectNB(ip, port);
		if (doRegister) registerConnector(reactor);
	}

	virtual ~SocketConnector()
	{
		try
		{
			unregisterConnector();
		}
		catch (...)
		{
		}
	}

	void registerConnector(SocketReactor& reactor)
	{
		_pReactor = &reactor;
		_pReactor->addEventHandler(_socket,
			Observer<SocketConnector, ReadableNotification>(*this, &SocketConnector::onReadable));
		_pReactor->addEventHandler(_socket,
			Observer<SocketConnector, WritableNotification>(*this, &SocketConnector::onWritable));
		_pReactor->addEventHandler(_socket,
			Observer<SocketConnector, ErrorNotification>(*this, &SocketConnector::onError));
	}

	void unregisterConnector()
	{
		if (_pReactor)
		{
			_pReactor->removeEventHandler(_socket,
				Observer<SocketConnector, ReadableNotification>(*this, &SocketConnector::onReadable));
			_pReactor->removeEventHandler(_socket,
				Observer<SocketConnector, WritableNotification>(*this, &SocketConnector::onWritable));
			_pReactor->removeEventHandler(_socket,
				Observer<SocketConnector, ErrorNotification>(*this, &SocketConnector::onError));
		}
	}

	void onReadable()
	{
		int err = _socket.socketError();
		if (err)
		{
			onError(err);
			unregisterConnector();
		}
		else
		{
			onConnect();
		}
	}

	void onWritable()
	{
		onConnect();
	}

	void onConnect()
	{
		_socket.setBlocking(true);
		createServiceHandler();
		unregisterConnector();
	}

	void onError()
	{
		onError(_socket.socketError());
		unregisterConnector();
	}

protected:
	virtual ServiceHandler* createServiceHandler()
	{
		return new ServiceHandler(_socket, *_pReactor);
	}

	virtual void onError(int errorCode)
	{
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
