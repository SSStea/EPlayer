#pragma once
#include <sys/un.h>
#include <string>

class Buffer : public std::string
{
public:
	Buffer() : std::string() {}
	Buffer(size_t size);
	Buffer(const std::string& str);
	Buffer(const char* str);
	Buffer(const char* str, size_t length);
	Buffer(const char* begin, const char* end);

	operator void* ();

	operator char* ();

	operator unsigned char* ();

	operator char* () const;

	operator const char* () const;

	operator const void* () const;
};

