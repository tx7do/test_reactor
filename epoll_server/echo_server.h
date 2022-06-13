#pragma once

#include <memory>
#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <mutex>

#include "server_socket.h"
#include "socket_reactor.h"
#include "socket_acceptor.h"
#include "server_application.h"
#include "fifo_buffer.h"
#include "observer.h"
#include "socket_notification.h"

#define SERVER_PORT   8080

class EchoServiceHandler
{
public:
	EchoServiceHandler(ServerSocket& socket, SocketReactor& reactor)
		: _socket(socket), _reactor(reactor), _fifoIn(BUFFER_SIZE, true), _fifoOut(BUFFER_SIZE, true)
	{
		_reactor.addEventHandler(_socket,
			NObserver<EchoServiceHandler, ReadableNotification>(*this, &EchoServiceHandler::onSocketReadable));
		_reactor.addEventHandler(_socket,
			NObserver<EchoServiceHandler, ShutdownNotification>(*this, &EchoServiceHandler::onSocketShutdown));

		_connIn = _fifoIn.writable.connect([this](bool b)
		{ onFIFOInWritable(b); });
		_connOut = _fifoOut.readable.connect([this](bool b)
		{ onFIFOOutReadable(b); });
	}

	~EchoServiceHandler()
	{
		_reactor.removeEventHandler(_socket,
			NObserver<EchoServiceHandler, ReadableNotification>(*this, &EchoServiceHandler::onSocketReadable));
		_reactor.removeEventHandler(_socket,
			NObserver<EchoServiceHandler, WritableNotification>(*this, &EchoServiceHandler::onSocketWritable));
		_reactor.removeEventHandler(_socket,
			NObserver<EchoServiceHandler, ShutdownNotification>(*this, &EchoServiceHandler::onSocketShutdown));

		_connIn.disconnect();
		_connOut.disconnect();
	}

public:
	void onFIFOOutReadable(bool& b)
	{
		if (b)
			_reactor.addEventHandler(_socket,
				NObserver<EchoServiceHandler, WritableNotification>(*this, &EchoServiceHandler::onSocketWritable));
		else
			_reactor.removeEventHandler(_socket,
				NObserver<EchoServiceHandler, WritableNotification>(*this, &EchoServiceHandler::onSocketWritable));
	}

	void onFIFOInWritable(bool& b)
	{
		if (b)
			_reactor.addEventHandler(_socket,
				NObserver<EchoServiceHandler, ReadableNotification>(*this, &EchoServiceHandler::onSocketReadable));
		else
			_reactor.removeEventHandler(_socket,
				NObserver<EchoServiceHandler, ReadableNotification>(*this, &EchoServiceHandler::onSocketReadable));
	}

	void onSocketReadable(const std::shared_ptr<ReadableNotification>& pNf)
	{
		try
		{
			ssize_t len = _socket.receiveBytes(_fifoIn);
			if (len > 0)
			{
				_fifoIn.drain(_fifoOut.write(_fifoIn.buffer(), _fifoIn.used()));
			}
			else
			{
				delete this;
			}
		}
		catch (const std::exception& exc)
		{
			delete this;
		}
	}

	void onSocketWritable(const std::shared_ptr<WritableNotification>& pNf)
	{
		try
		{
			_socket.sendBytes(_fifoOut);
		}
		catch (const std::exception& exc)
		{
			delete this;
		}
	}

	void onSocketShutdown(const std::shared_ptr<ShutdownNotification>& pNf)
	{
		delete this;
	}

private:
	enum
	{
		BUFFER_SIZE = 1024
	};

	ServerSocket _socket;
	SocketReactor& _reactor;

	FIFOBuffer _fifoIn;
	FIFOBuffer _fifoOut;

	boost::signals2::connection _connOut;
	boost::signals2::connection _connIn;
};

class EchoServer : public ServerApplication
{
public:
	EchoServer() = default;
	~EchoServer() = default;

protected:
	void initialize(ServerApplication& self) final
	{
		ServerApplication::initialize(self);
	}

	void uninitialize() final
	{
		ServerApplication::uninitialize();
	}

	int main(const std::vector<std::string>& args) final
	{
		unsigned short port = SERVER_PORT;

		ServerSocket svs(port);

		SocketReactor reactor;

		SocketAcceptor<EchoServiceHandler> acceptor(svs, reactor);

		std::thread thread([&reactor]()
		{ reactor.run(); });

		waitForTerminationRequest();

		reactor.stop();
		thread.join();

		return ServerApplication::EXIT_OK;
	}
};
