#pragma once

#include <cassert>
#include <cstring>
#include <cstddef>
#include <exception>
#include <stdexcept>

template<class T>
class Buffer
{
public:
	Buffer() = delete;

	explicit Buffer(std::size_t length)
		: _capacity(length), _used(length), _ptr(nullptr), _ownMem(true)
	{
		if (length > 0)
		{
			_ptr = new T[length];
		}
	}

	Buffer(T* pMem, std::size_t length)
		: _capacity(length), _used(length), _ptr(pMem), _ownMem(false)
	{
	}

	Buffer(const T* pMem, std::size_t length)
		: _capacity(length), _used(length), _ptr(nullptr), _ownMem(true)
	{
		if (_capacity > 0)
		{
			_ptr = new T[_capacity];
			std::memcpy(_ptr, pMem, _used * sizeof(T));
		}
	}

	Buffer(const Buffer& other)
		: _capacity(other._used), _used(other._used), _ptr(nullptr), _ownMem(true)
	{
		if (_used)
		{
			_ptr = new T[_used];
			std::memcpy(_ptr, other._ptr, _used * sizeof(T));
		}
	}

	Buffer(Buffer&& other) noexcept
		: _capacity(other._capacity), _used(other._used), _ptr(other._ptr), _ownMem(other._ownMem)
	{
		other._capacity = 0;
		other._used = 0;
		other._ownMem = false;
		other._ptr = nullptr;
	}

	Buffer& operator=(const Buffer& other)
	{
		if (this != &other)
		{
			Buffer tmp(other);
			swap(tmp);
		}

		return *this;
	}

	Buffer& operator=(Buffer&& other) noexcept
	{
		if (_ownMem) delete[] _ptr;

		_capacity = other._capacity;
		_used = other._used;
		_ptr = other._ptr;
		_ownMem = other._ownMem;

		other._capacity = 0;
		other._used = 0;
		other._ownMem = false;
		other._ptr = nullptr;

		return *this;
	}

	~Buffer()
	{
		if (_ownMem && _ptr != nullptr)
		{
			delete[] _ptr;
			_ptr = nullptr;
		}
	}

	/// 改变当前Buffer的大小为newSize
	/// @param [in] newCapacity 新的缓存大小
	/// @param [in] preserveContent 是否保留已有数据标志
	void resize(std::size_t newCapacity, bool preserveContent = true)
	{
		if (!_ownMem) throw std::runtime_error("Cannot resize buffer which does not own its storage.");

		assert(newCapacity > 0);
		if (newCapacity <= 0) return;

		if (newCapacity > _capacity)
		{
			T* ptr = new T[newCapacity];
			if (preserveContent && _ptr)
			{
				std::memcpy(ptr, _ptr, _used * sizeof(T));
			}
			delete[] _ptr;
			_ptr = ptr;
			_capacity = newCapacity;
		}

		_used = newCapacity;
	}

	void setCapacity(std::size_t newCapacity, bool preserveContent = true)
	{
		if (!_ownMem) throw std::runtime_error("Cannot resize buffer which does not own its storage.");

		if (newCapacity != _capacity)
		{
			T* ptr = 0;
			if (newCapacity > 0)
			{
				ptr = new T[newCapacity];
				if (preserveContent && _ptr)
				{
					std::size_t newSz = _used < newCapacity ? _used : newCapacity;
					std::memcpy(ptr, _ptr, newSz * sizeof(T));
				}
			}
			delete[] _ptr;
			_ptr = ptr;
			_capacity = newCapacity;

			if (newCapacity < _used) _used = newCapacity;
		}
	}

	void assign(const T* buf, std::size_t sz)
	{
		if (0 == sz) return;
		if (sz > _capacity) resize(sz, false);
		std::memcpy(_ptr, buf, sz * sizeof(T));
		_used = sz;
	}

	void append(const T* buf, std::size_t sz)
	{
		if (0 == sz) return;
		resize(_used + sz, true);
		std::memcpy(_ptr + _used - sz, buf, sz * sizeof(T));
	}

	void append(T val)
	{
		resize(_used + 1, true);
		_ptr[_used - 1] = val;
	}

	void append(const Buffer& buf)
	{
		append(buf.begin(), buf.size());
	}

	/// 缓存可容纳的元素量
	std::size_t capacity() const
	{
		return _capacity;
	}

	std::size_t capacityBytes() const
	{
		return _capacity * sizeof(T);
	}

	/// 缓存的数据大小
	std::size_t size() const
	{
		return _used;
	}

	std::size_t sizeBytes() const
	{
		return _used * sizeof(T);
	}

	bool empty() const
	{
		return 0 == _used;
	}

	void swap(Buffer& other)
	{
		using std::swap;

		swap(_ptr, other._ptr);
		swap(_capacity, other._capacity);
		swap(_used, other._used);
		swap(_ownMem, other._ownMem);
	}

	bool operator==(const Buffer& other) const
	{
		if (this != &other)
		{
			if (_used == other._used)
			{
				if (_ptr && other._ptr && std::memcmp(_ptr, other._ptr, _used * sizeof(T)) == 0)
				{
					return true;
				}
				else return _used == 0;
			}
			return false;
		}

		return true;
	}

	bool operator!=(const Buffer& other) const
	{
		return *this != other;
	}

	void clear()
	{
		std::memset(_ptr, 0, _used * sizeof(T));
	}

	/// 缓存的起始指针
	T* begin()
	{
		return _ptr;
	}
	const T* begin() const
	{
		return _ptr;
	}

	/// 缓存的尾指针
	T* end()
	{
		return _ptr + _used;
	}
	const T* end() const
	{
		return _ptr + _used;
	}

	T& operator[](std::size_t index)
	{
		assert (index < _used);

		return _ptr[index];
	}

	const T& operator[](std::size_t index) const
	{
		assert (index < _used);

		return _ptr[index];
	}

private:
	std::size_t _capacity;
	std::size_t _used;
	T* _ptr;
	bool _ownMem;
};
