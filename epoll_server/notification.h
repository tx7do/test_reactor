#pragma once

#include <string>
#include <memory>

class Notification
{
public:
	using Ptr = std::unique_ptr<Notification>;

	Notification() = default;
	virtual ~Notification() = default;

	virtual std::string name() const;
};
