#include "Public.h"



Buffer::Buffer(size_t size) : std::string()
{
	resize(size);
}

Buffer::Buffer(const std::string& str) : std::string(str)
{
}

Buffer::Buffer(const char* str) : std::string(str)
{
}

Buffer::Buffer(const char* str, size_t length) : std::string(str)
{
	resize(length);
	memcpy((char*)c_str(), str, length);
}

Buffer::Buffer(const char* begin, const char* end) : std::string()
{
	ptrdiff_t nLength = end - begin; // °üº¬ begin£¬²»°üº¬ end
	if (nLength > 0)
	{
		resize((size_t)nLength);
		memcpy((char*)c_str(), begin, (size_t)nLength);
	}
}

Buffer::operator char* ()
{
	return (char*)c_str();
}

Buffer::operator char* () const
{
	return (char*)c_str();
}

Buffer::operator const char* () const
{
	return c_str();
}
