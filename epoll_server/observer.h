#pragma once

#include <memory>
#include <mutex>
#include "notification.h"


class AbstractObserver
{
public:
	typedef std::shared_ptr<AbstractObserver> Ptr;
public:
	AbstractObserver() = default;
	AbstractObserver(const AbstractObserver& observer) = default;
	virtual ~AbstractObserver() = default;

	AbstractObserver& operator=(const AbstractObserver& observer) = default;

	virtual void notify(Notification* pNf) const = 0;
	virtual bool equals(const AbstractObserver& observer) const = 0;
	virtual bool accepts(Notification* pNf, const char* pName = nullptr) const = 0;
	virtual AbstractObserver::Ptr clone() const = 0;
	virtual void disable() = 0;
};

template<class C, class N>
class Observer : public AbstractObserver
{
public:
	typedef void (C::*Callback)(N*);

	Observer() = delete;

	Observer(C& object, Callback method)
		: _pObject(&object), _method(method)
	{
	}

	Observer(const Observer& observer)
		: AbstractObserver(observer), _pObject(observer._pObject), _method(observer._method)
	{
	}

	~Observer() override = default;

	Observer& operator=(const Observer& observer)
	{
		if (&observer != this)
		{
			_pObject = observer._pObject;
			_method = observer._method;
		}
		return *this;
	}

	void notify(Notification* pNf) const override
	{
		std::unique_lock<std::mutex> lock(_mutex);

		if (_pObject)
		{
			N* pCastNf = dynamic_cast<N*>(pNf);
			if (pCastNf)
			{
//				pCastNf->duplicate();
				(_pObject->*_method)(pCastNf);
			}
		}
	}

	bool equals(const AbstractObserver& abstractObserver) const override
	{
		const auto* pObs = dynamic_cast<const Observer*>(&abstractObserver);
		return pObs && pObs->_pObject == _pObject && pObs->_method == _method;
	}

	bool accepts(Notification* pNf, const char* pName) const override
	{
		return dynamic_cast<N*>(pNf) && (!pName || pNf->name() == pName);
	}

	AbstractObserver::Ptr clone() const override
	{
		return new Observer(*this);
	}

	void disable() override
	{
		std::unique_lock<std::mutex> lock(_mutex);

		_pObject = 0;
	}

private:
	C* _pObject;
	Callback _method;
	mutable std::mutex _mutex;
};

template<class C, class N>
class NObserver : public AbstractObserver
{
public:
	typedef std::shared_ptr<N> NotificationPtr;
	typedef void (C::*Callback)(const NotificationPtr&);

	NObserver() = delete;

	NObserver(C& object, Callback method)
		: _pObject(&object), _method(method)
	{
	}

	NObserver(const NObserver& observer)
		: AbstractObserver(observer), _pObject(observer._pObject), _method(observer._method)
	{
	}

	~NObserver() override = default;

	NObserver& operator=(const NObserver& observer)
	{
		if (&observer != this)
		{
			_pObject = observer._pObject;
			_method = observer._method;
		}
		return *this;
	}

	void notify(Notification* pNf) const override
	{
		std::unique_lock<std::mutex> lock(_mutex);

		if (_pObject)
		{
			N* pCastNf = dynamic_cast<N*>(pNf);
			if (pCastNf)
			{
				NotificationPtr ptr(pCastNf, true);
				(_pObject->*_method)(ptr);
			}
		}
	}

	bool equals(const AbstractObserver& abstractObserver) const override
	{
		const auto* pObs = dynamic_cast<const NObserver*>(&abstractObserver);
		return pObs && pObs->_pObject == _pObject && pObs->_method == _method;
	}

	bool accepts(Notification* pNf, const char* pName) const override
	{
		return dynamic_cast<N*>(pNf) && (!pName || pNf->name() == pName);
	}

	AbstractObserver::Ptr clone() const override
	{
		return new NObserver(*this);
	}

	void disable() override
	{
		std::unique_lock<std::mutex> lock(_mutex);
		_pObject = nullptr;
	}

private:
	C* _pObject;
	Callback _method;
	mutable std::mutex _mutex;
};
