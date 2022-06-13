#include "socket_reactor.h"
#include <memory>

SocketReactor::SocketReactor()
	: _stop(false), _pThread(nullptr), _timeout(std::chrono::microseconds(DEFAULT_TIMEOUT))
{
}

SocketReactor::~SocketReactor() = default;

void SocketReactor::run()
{
	_pThread = nullptr;
	while (!_stop)
	{
		try
		{
			if (!hasSocketHandlers())
			{
				onIdle();
				std::this_thread::sleep_for(_timeout);
			}
			else
			{
				bool readable = false;
				PollSet::SocketModeMap sm = _pollSet.poll(_timeout);
				if (sm.size() > 0)
				{
					onBusy();
					auto it = sm.begin();
					auto end = sm.end();
					for (; it != end; ++it)
					{
						if (it->second & PollSet::POLL_READ)
						{
							dispatch(it->first, _pReadableNotification.get());
							readable = true;
						}
						if (it->second & PollSet::POLL_WRITE) dispatch(it->first, _pWritableNotification.get());
						if (it->second & PollSet::POLL_ERROR) dispatch(it->first, _pErrorNotification.get());
					}
				}
				if (!readable) onTimeout();
			}
		}
		catch (std::exception& exc)
		{
		}
		catch (...)
		{
		}
	}
	onShutdown();
}

bool SocketReactor::hasSocketHandlers()
{
	if (!_pollSet.empty())
	{
		std::lock_guard<std::mutex> guard(_mutex);
		for (auto& p : _handlers)
		{
			if (p.second->accepts(_pReadableNotification.get()) ||
				p.second->accepts(_pWritableNotification.get()) ||
				p.second->accepts(_pErrorNotification.get()))
				return true;
		}
	}

	return false;
}

void SocketReactor::stop()
{
	_stop = true;
}

void SocketReactor::wakeUp()
{
//	if (_pThread) _pThread->wakeUp();
}

void SocketReactor::setTimeout(const std::chrono::system_clock::duration& timeout)
{
	_timeout = timeout;
}

const std::chrono::system_clock::duration& SocketReactor::getTimeout() const
{
	return _timeout;
}

void SocketReactor::addEventHandler(const ServerSocket& socket, const AbstractObserver& observer)
{
	NotifierPtr pNotifier = getNotifier(socket, true);

	if (!pNotifier->hasObserver(observer)) pNotifier->addObserver(this, observer);

	int mode = 0;
	if (pNotifier->accepts(_pReadableNotification.get())) mode |= PollSet::POLL_READ;
	if (pNotifier->accepts(_pWritableNotification.get())) mode |= PollSet::POLL_WRITE;
	if (pNotifier->accepts(_pErrorNotification.get())) mode |= PollSet::POLL_ERROR;
	if (mode) _pollSet.add(socket, mode);
}

bool SocketReactor::hasEventHandler(const ServerSocket& socket, const AbstractObserver& observer)
{
	NotifierPtr pNotifier = getNotifier(socket);
	if (!pNotifier) return false;
	if (pNotifier->hasObserver(observer)) return true;
	return false;
}

SocketReactor::NotifierPtr SocketReactor::getNotifier(const ServerSocket& socket, bool makeNew)
{
	std::lock_guard<std::mutex> guard(_mutex);

	auto it = _handlers.find(socket);
	if (it != _handlers.end())
	{
		return it->second;
	}
	else if (makeNew)
	{
		return (_handlers[socket] = std::make_shared<SocketNotifier>(socket));
//		return (_handlers[socket] = new SocketNotifier(socket));
	}

	return nullptr;
}

void SocketReactor::removeEventHandler(const ServerSocket& socket, const AbstractObserver& observer)
{
	NotifierPtr pNotifier = getNotifier(socket);
	if (pNotifier && pNotifier->hasObserver(observer))
	{
		if (pNotifier->countObservers() == 1)
		{
			{
				std::lock_guard<std::mutex> guard(_mutex);
				_handlers.erase(socket);
			}
			_pollSet.remove(socket);
		}
		pNotifier->removeObserver(this, observer);

		if (pNotifier->countObservers() > 0 && socket.sockfd() > 0)
		{
			int mode = 0;
			if (pNotifier->accepts(_pReadableNotification.get())) mode |= PollSet::POLL_READ;
			if (pNotifier->accepts(_pWritableNotification.get())) mode |= PollSet::POLL_WRITE;
			if (pNotifier->accepts(_pErrorNotification.get())) mode |= PollSet::POLL_ERROR;
			_pollSet.update(socket, mode);
		}
	}
}

bool SocketReactor::has(const ServerSocket& socket) const
{
	return _pollSet.has(socket);
}

void SocketReactor::onTimeout()
{
	dispatch(_pTimeoutNotification.get());
}

void SocketReactor::onIdle()
{
	dispatch(_pIdleNotification.get());
}

void SocketReactor::onShutdown()
{
	dispatch(_pShutdownNotification.get());
}

void SocketReactor::onBusy()
{
}

void SocketReactor::dispatch(const ServerSocket& socket, SocketNotification* pNotification)
{
	NotifierPtr pNotifier = getNotifier(socket);
	if (!pNotifier) return;
	dispatch(pNotifier, pNotification);
}

void SocketReactor::dispatch(SocketNotification* pNotification)
{
	std::vector<NotifierPtr> delegates;
	{
		std::lock_guard<std::mutex> guard(_mutex);
		delegates.reserve(_handlers.size());
		for (auto& _handler : _handlers)
			delegates.push_back(_handler.second);
	}
	for (auto& delegate : delegates)
	{
		dispatch(delegate, pNotification);
	}
}

void SocketReactor::dispatch(NotifierPtr& pNotifier, SocketNotification* pNotification)
{
	try
	{
		pNotifier->dispatch(pNotification);
	}
	catch (std::exception& exc)
	{
	}
	catch (...)
	{
	}
}
