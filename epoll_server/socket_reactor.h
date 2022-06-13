#pragma once

#include <map>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <ctime>
#include <boost/signals2.hpp>

#include "poll_set.h"
#include "socket_notifier.h"

class ServerSocket;

class EventHandler
{
public:
	virtual ~EventHandler() = default;

	virtual void onAccept() = 0;
	virtual void onConnect() = 0;

	virtual void onError() = 0;

	virtual void onReadable() = 0;
	virtual void onWritable() = 0;
};

typedef std::shared_ptr<EventHandler> EventHandlerPtr;

class SocketReactor
{
	friend class SocketNotifier;
	typedef std::shared_ptr<SocketNotifier> NotifierPtr;
	typedef std::shared_ptr<SocketNotification> NotificationPtr;
	typedef std::map<ServerSocket, NotifierPtr> EventHandlerMap;

public:
	SocketReactor();
	virtual ~SocketReactor();

public:
	void run();

	void stop();

	void wakeUp();

	void addEventHandler(const ServerSocket& socket, const AbstractObserver& observer);
	bool hasEventHandler(const ServerSocket& socket, const AbstractObserver& observer);
	void removeEventHandler(const ServerSocket& socket, const AbstractObserver& observer);

	bool has(const ServerSocket& socket) const;

	void setTimeout(const std::chrono::system_clock::duration& timeout);
	const std::chrono::system_clock::duration& getTimeout() const;

protected:
	void onTimeout();

	void onIdle();

	void onShutdown();

	void onBusy();

protected:
	void dispatch(const ServerSocket& socket, SocketNotification* pNotification);

	void dispatch(SocketNotification* pNotification);

	void dispatch(NotifierPtr& pNotifier, SocketNotification* pNotification);

	bool hasSocketHandlers();

	NotifierPtr getNotifier(const ServerSocket& socket, bool makeNew = false);

private:
	enum
	{
		DEFAULT_TIMEOUT = 250000
	};

	std::atomic<bool> _stop;
	mutable std::mutex _mutex;
	std::thread* _pThread;
	std::chrono::system_clock::duration _timeout;

	PollSet _pollSet;

private:
	EventHandlerMap _handlers;

private:
	NotificationPtr _pReadableNotification;
	NotificationPtr _pWritableNotification;
	NotificationPtr _pErrorNotification;
	NotificationPtr _pTimeoutNotification;
	NotificationPtr _pIdleNotification;
	NotificationPtr _pShutdownNotification;
};
