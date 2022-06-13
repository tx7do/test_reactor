#include "notification.h"

std::string Notification::name() const
{
	return typeid(*this).name();
}
