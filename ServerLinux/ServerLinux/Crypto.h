#pragma once
#include "Public.h"
#include <openssl/md5.h>

class CCrypto
{
public:
	static Buffer MD5(const Buffer& text);
};

