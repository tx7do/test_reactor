#pragma once

#include <set>
#include <memory>
#include <mutex>

#include "server_socket.h"
#include "notification.h"
#include "notification_center.h"

class SocketReactor;
class AbstractObserver;
class SocketNotification;

class SocketNotifier
{
public:
	explicit SocketNotifier(const ServerSocket& socket);
	~SocketNotifier() = default;

	void addObserver(SocketReactor* pReactor, const AbstractObserver& observer);

	void removeObserver(SocketReactor* pReactor, const AbstractObserver& observer);

	bool hasObserver(const AbstractObserver& observer) const;

	bool accepts(SocketNotification* pNotification);

	void dispatch(SocketNotification* pNotification);

	bool hasObservers() const;

	std::size_t countObservers() const;

private:
	typedef std::multiset<SocketNotification*> EventSet;

	EventSet _events;
	NotificationCenter _nc;
	ServerSocket _socket;
	std::mutex _mutex;
};

//
// inlines
//
inline bool SocketNotifier::accepts(SocketNotification* pNotification)
{
	std::unique_lock<std::mutex> lock(_mutex);
	return _events.find(pNotification) != _events.end();
}

inline bool SocketNotifier::hasObserver(const AbstractObserver& observer) const
{
	return _nc.hasObserver(observer);
}

inline bool SocketNotifier::hasObservers() const
{
	return _nc.hasObservers();
}

inline std::size_t SocketNotifier::countObservers() const
{
	return _nc.countObservers();
}
