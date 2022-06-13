#pragma once

#include <cstdlib>
#include <mutex>
#include <cstdio>
#include <boost/format.hpp>
#include <boost/signals2.hpp>

#include "buffer.h"

template<class T>
class BasicFIFOBuffer
{
public:
	typedef T Type;

	mutable boost::signals2::signal<void(bool)> writable;
	mutable boost::signals2::signal<void(bool)> readable;

	BasicFIFOBuffer() = delete;
	BasicFIFOBuffer(const BasicFIFOBuffer&) = delete;
	BasicFIFOBuffer& operator=(const BasicFIFOBuffer&) = delete;

	explicit BasicFIFOBuffer(std::size_t size, bool notify = false)
		: _buffer(size), _begin(0), _used(0), _notify(notify), _eof(false), _error(false)
	{
	}

	BasicFIFOBuffer(T* pBuffer, std::size_t size, bool notify = false)
		: _buffer(pBuffer, size), _begin(0), _used(0), _notify(notify), _eof(false), _error(false)
	{
	}

	BasicFIFOBuffer(const T* pBuffer, std::size_t size, bool notify = false)
		: _buffer(pBuffer, size), _begin(0), _used(size), _notify(notify), _eof(false), _error(false)
	{
	}

	~BasicFIFOBuffer() = default;

	void resize(std::size_t newSize, bool preserveContent = true)
	{
		std::unique_lock<std::mutex> lock(_mutex);

		if (preserveContent && (newSize < _used))
			throw std::runtime_error("Can not resize FIFO without data loss.");

		std::size_t usedBefore = _used;
		_buffer.resize(newSize, preserveContent);
		if (!preserveContent) _used = 0;
		if (_notify) notify(usedBefore);
	}

	std::size_t peek(T* pBuffer, std::size_t length) const
	{
		if (0 == length) return 0;
		std::unique_lock<std::mutex> lock(_mutex);
		if (!isReadable()) return 0;
		if (length > _used) length = _used;
		std::memcpy(pBuffer, _buffer.begin() + _begin, length * sizeof(T));
		return length;
	}

	std::size_t peek(Buffer<T>& buffer, std::size_t length = 0) const
	{
		std::unique_lock<std::mutex> lock(_mutex);
		if (!isReadable()) return 0;
		if (0 == length || length > _used) length = _used;
		buffer.resize(length);
		return peek(buffer.begin(), length);
	}

	std::size_t read(T* pBuffer, std::size_t length)
	{
		if (0 == length) return 0;
		std::unique_lock<std::mutex> lock(_mutex);
		if (!isReadable()) return 0;
		std::size_t usedBefore = _used;
		std::size_t readLen = peek(pBuffer, length);
		assert(_used >= readLen);
		_used -= readLen;
		if (0 == _used) _begin = 0;
		else _begin += length;

		if (_notify) notify(usedBefore);

		return readLen;
	}

	std::size_t read(Buffer<T>& buffer, std::size_t length = 0)
	{
		std::unique_lock<std::mutex> lock(_mutex);
		if (!isReadable()) return 0;
		std::size_t usedBefore = _used;
		std::size_t readLen = peek(buffer, length);
		assert(_used >= readLen);
		_used -= readLen;
		if (0 == _used) _begin = 0;
		else _begin += length;

		if (_notify) notify(usedBefore);

		return readLen;
	}

	std::size_t write(const T* pBuffer, std::size_t length)
	{
		if (0 == length) return 0;

		std::unique_lock<std::mutex> lock(_mutex);

		if (!isWritable()) return 0;

		if (_buffer.size() - (_begin + _used) < length)
		{
			std::memmove(_buffer.begin(), begin(), _used * sizeof(T));
			_begin = 0;
		}

		std::size_t usedBefore = _used;
		std::size_t available = _buffer.size() - _used - _begin;
		std::size_t len = length > available ? available : length;
		std::memcpy(begin() + _used, pBuffer, len * sizeof(T));
		_used += len;
		assert(_used <= _buffer.size());
		if (_notify) notify(usedBefore);

		return len;
	}

	std::size_t write(const Buffer<T>& buffer, std::size_t length = 0)
	{
		if (length == 0 || length > buffer.size())
			length = buffer.size();

		return write(buffer.begin(), length);
	}

	std::size_t size() const
	{
		return _buffer.size();
	}

	std::size_t used() const
	{
		return _used;
	}

	std::size_t available() const
	{
		return size() - _used;
	}

	void drain(std::size_t length = 0)
	{
		std::unique_lock<std::mutex> lock(_mutex);

		std::size_t usedBefore = _used;

		if (0 == length || length >= _used)
		{
			_begin = 0;
			_used = 0;
		}
		else
		{
			_begin += length;
			_used -= length;
		}

		if (_notify) notify(usedBefore);
	}

	void copy(const T* ptr, std::size_t length)
	{
		poco_check_ptr(ptr);
		if (0 == length) return;

		std::unique_lock<std::mutex> lock(_mutex);

		if (length > available())
		{
			throw std::runtime_error("Cannot extend buffer.");
		}

		if (!isWritable())
		{
			throw std::runtime_error("Buffer not writable.");
		}

		std::memcpy(begin() + _used, ptr, length * sizeof(T));
		std::size_t usedBefore = _used;
		_used += length;
		if (_notify) notify(usedBefore);
	}

	void advance(std::size_t length)
	{
		std::unique_lock<std::mutex> lock(_mutex);

		if (length > available())
		{
			throw std::runtime_error("Cannot extend buffer.");
		}

		if (!isWritable())
		{
			throw std::runtime_error("Buffer not writable.");
		}

		if (_buffer.size() - (_begin + _used) < length)
		{
			std::memmove(_buffer.begin(), begin(), _used * sizeof(T));
			_begin = 0;
		}

		std::size_t usedBefore = _used;
		_used += length;
		if (_notify) notify(usedBefore);
	}

	T* begin()
	{
		std::unique_lock<std::mutex> lock(_mutex);
		if (_begin != 0)
		{
			std::memmove(_buffer.begin(), _buffer.begin() + _begin, _used * sizeof(T));
			_begin = 0;
		}
		return _buffer.begin();
	}

	T* next()
	{
		std::unique_lock<std::mutex> lock(_mutex);
		return begin() + _used;
	}

	T& operator[](std::size_t index)
	{
		std::unique_lock<std::mutex> lock(_mutex);
		if (index >= _used)
		{
			throw std::runtime_error((boost::format("Index out of bounds: %z (max index allowed: %z)") % index, _used
				- 1));
		}

		return _buffer[_begin + index];
	}

	const T& operator[](std::size_t index) const
	{
		std::unique_lock<std::mutex> lock(_mutex);
		if (index >= _used)
		{
			throw std::runtime_error((boost::format("Index out of bounds: %z (max index allowed: %z)") % index, _used
				- 1));
		}

		return _buffer[_begin + index];
	}

	const Buffer<T>& buffer() const
	{
		return _buffer;
	}

	void setError(bool error = true)
	{
		if (error)
		{
			bool f = false;
			std::unique_lock<std::mutex> lock(_mutex);
			if (isReadable() && _notify) readable(f);
			if (isWritable() && _notify) writable(f);
			_error = error;
			_used = 0;
		}
		else
		{
			bool t = true;
			std::unique_lock<std::mutex> lock(_mutex);
			_error = false;
			if (_notify && !_eof) writable(t);
		}
	}

	bool isValid() const
	{
		return !_error;
	}

	void setEOF(bool eof = true)
	{
		std::unique_lock<std::mutex> lock(_mutex);
		bool flag = !eof;
		if (_notify) writable(flag);
		_eof = eof;
	}

	bool hasEOF() const
	{
		return _eof;
	}

	bool isEOF() const
	{
		return isEmpty() && _eof;
	}

	bool isEmpty() const
	{
		return 0 == _used;
	}

	bool isFull() const
	{
		return size() == _used;
	}

	bool isReadable() const
	{
		return !isEmpty() && isValid();
	}

	bool isWritable() const
	{
		return !isFull() && isValid() && !_eof;
	}

	void setNotify(bool notify = true)
	{
		_notify = notify;
	}

	bool getNotify() const
	{
		return _notify;
	}

	std::mutex& mutex()
	{
		return _mutex;
	}

private:
	void notify(std::size_t usedBefore)
	{
		bool t = true, f = false;

		if (usedBefore == 0 && _used > 0)
		{
			readable(t);
		}
		else if (usedBefore > 0 && 0 == _used)
		{
			readable(f);
		}

		if (usedBefore == _buffer.size() && _used < _buffer.size())
		{
			writable(t);
		}
		else if (usedBefore < _buffer.size() && _used == _buffer.size())
		{
			writable(f);
		}
	}

private:
	Buffer<T> _buffer;

	std::size_t _begin;
	std::size_t _used;

	mutable std::mutex _mutex;

	bool _notify;
	bool _eof;
	bool _error;
};

typedef BasicFIFOBuffer<char> FIFOBuffer;
