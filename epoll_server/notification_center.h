#pragma once

#include <vector>
#include <mutex>
#include <memory>

#include "notification.h"
#include "observer.h"


class NotificationCenter
{
public:
	NotificationCenter() = default;
	~NotificationCenter() = default;

	static NotificationCenter& defaultCenter();

	void addObserver(const AbstractObserver& observer);

	void removeObserver(const AbstractObserver& observer);

	bool hasObserver(const AbstractObserver& observer) const;

	bool hasObservers() const;

	std::size_t countObservers() const;

	void postNotification(const Notification::Ptr& pNotification);

private:
	typedef AbstractObserver::Ptr AbstractObserverPtr;
	typedef std::vector<AbstractObserverPtr> ObserverList;

	ObserverList _observers;
	mutable std::mutex _mutex;

	static NotificationCenter _defaultCenter;
};
