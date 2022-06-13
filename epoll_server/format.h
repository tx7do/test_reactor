#pragma once

#include <vector>
#include <string>
#include <cstdarg>
#include <cstdio>

static std::string format(const char *fmt, ...)
{
	using std::vector;

	std::string retStr;

	va_list arg_ptr;

	va_start(arg_ptr, fmt);
	vsprintf(string, fmt, arg_ptr);
	va_end(arg_ptr);

	va_start(marker, fmt);

	size_t len = _vscprintf(fmt, marker) + 1;

	std::vector<char> buffer(len, '\0');
	int nWritten = _vsnprintf(&buffer[0], buffer.size(), fmt, marker);
	if (nWritten > 0)
	{
		retStr = &buffer[0];
	}

	va_end(marker);
	return retStr;
}
