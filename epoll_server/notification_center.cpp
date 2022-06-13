
#include "notification_center.h"
#include "notification.h"
#include "observer.h"


void NotificationCenter::addObserver(const AbstractObserver& observer)
{
	std::unique_lock<std::mutex> lock(_mutex);
	_observers.push_back(observer.clone());
}

void NotificationCenter::removeObserver(const AbstractObserver& observer)
{
	std::unique_lock<std::mutex> lock(_mutex);
	for (auto it = _observers.begin(); it != _observers.end(); ++it)
	{
		if (observer.equals(**it))
		{
			(*it)->disable();
			_observers.erase(it);
			return;
		}
	}
}

bool NotificationCenter::hasObserver(const AbstractObserver& observer) const
{
	std::unique_lock<std::mutex> lock(_mutex);
	for (const auto& p : _observers)
	{
		if (observer.equals(*p)) return true;
	}

	return false;
}

void NotificationCenter::postNotification(const Notification::Ptr& pNotification)
{
	_mutex.lock();
	ObserverList observersToNotify(_observers);
	_mutex.unlock();

	for (auto& p : observersToNotify)
	{
		p->notify(pNotification.get());
	}
}

bool NotificationCenter::hasObservers() const
{
	std::unique_lock<std::mutex> lock(_mutex);

	return !_observers.empty();
}

std::size_t NotificationCenter::countObservers() const
{
	std::unique_lock<std::mutex> lock(_mutex);

	return _observers.size();
}

NotificationCenter NotificationCenter::_defaultCenter;

NotificationCenter& NotificationCenter::defaultCenter()
{
	return _defaultCenter;
}
