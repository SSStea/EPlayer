#include "Socket.h"

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

CSockParam::CSockParam()
{
	bzero(&addr_in, sizeof(addr_in));
	bzero(&addr_un, sizeof(addr_un));
	port = -1;
	attr = 0;//默认客户端、阻塞、TCP
}

CSockParam::CSockParam(const Buffer& ip, short port, int attr)
{
	this->ip = ip;
	this->port = port;
	this->attr = attr;
	addr_in.sin_family = AF_INET;
	addr_in.sin_port = port;
	addr_in.sin_addr.s_addr = inet_addr(ip);
}

CSockParam::CSockParam(const Buffer& path, int arrt)
{
	ip = path;
	this->attr = arrt;
	addr_un.sun_family = AF_UNIX;
	strcpy(addr_un.sun_path, path);
}

CSockParam::CSockParam(const CSockParam& param)
{
	ip = param.ip;
	port = param.port;
	attr = param.attr;
	memcpy(&addr_in, &param.addr_in, sizeof(addr_in));
	memcpy(&addr_un, &param.addr_un, sizeof(addr_un));
}

CSockParam& CSockParam::operator=(const CSockParam& param)
{
	if (this != &param)
	{
		ip = param.ip;
		port = param.port;
		attr = param.attr;
		memcpy(&addr_in, &param.addr_in, sizeof(addr_in));
		memcpy(&addr_un, &param.addr_un, sizeof(addr_un));
	}

	return *this;
}

sockaddr* CSockParam::addrin()
{
	return(sockaddr*)&addr_in;
}

sockaddr* CSockParam::addrun()
{
	return(sockaddr*)&addr_un;
}

CSocketBase::CSocketBase()
{
	m_socket = -1;
	m_status = 0;
}

CSocketBase::~CSocketBase()
{
	Close();
}

int CSocketBase::Close()
{
	m_status = 3;
	if (m_socket != -1)
	{
		unlink(m_param.ip);
		int fd = m_socket;
		m_socket = -1;
		close(fd);
	}
	return 0;
}

CSocketBase::operator int()
{
	return m_socket;
}
CSocketBase::operator int() const
{
	return m_socket;
}

CLocalSocket::CLocalSocket():CSocketBase()
{
}

CLocalSocket::CLocalSocket(int sock) : CSocketBase()
{
	m_socket = sock;
}

CLocalSocket::~CLocalSocket()
{
	Close();
}

int CLocalSocket::Init(const CSockParam& param)
{
	if (m_status != 0)
	{
		return -1;
	}
	m_param = param;
	int type = (m_param.attr & SOCK_ISUDP) ? SOCK_DGRAM : SOCK_STREAM;
	if(m_socket == -1)
	{
		m_socket = socket(PF_LOCAL, type, 0);
	}
	if (m_socket == -1)
	{
		return -2;
	}

	int nRet = 0;
	if (m_param.attr & SOCK_ISSERVER)//判断是否是服务器
	{
		nRet = bind(m_socket, m_param.addrun(), sizeof(sockaddr_un));
		if (nRet == -1)
		{
			if (errno != EAGAIN)
			{
				return -3;
			}
		}

		nRet = listen(m_socket, 32);
		if (nRet == -1)
		{
			return -4;
		}
	}

	if (m_param.attr & SOCK_ISNONBLOCK)//判断是否阻塞
	{
		int option = fcntl(m_socket, F_GETFL);
		if (option == -1)
		{
			return -5;
		}

		option |= O_NONBLOCK;
		nRet = fcntl(m_socket, F_SETFL, option);
		if (nRet == -1)
		{
			return -6;
		}
	}

	m_status = 1;
	return 0;
}

int CLocalSocket::Link(CSocketBase** pClient)
{
	if (m_status <= 0 || m_socket == -1)
	{
		return -1;
	}

	int nRet = 0;
	if (m_param.attr & SOCK_ISSERVER)//判断是否是服务器
	{
		if (pClient == NULL)
		{
			return -2;
		}

		CSockParam param;
		socklen_t len = sizeof(sockaddr_un);
		int nFd = accept(m_socket, param.addrun(), &len);
		if (nFd == -1)
		{
			return -3;
		}

		*pClient = new CLocalSocket(nFd);
		if (*pClient == NULL)
		{
			return -4;
		}
		nRet = (*pClient)->Init(param);
		if (nRet != 0)
		{
			delete *pClient;
			*pClient = NULL;
			return -5;
		}
	}
	else
	{
		nRet = connect(m_socket, m_param.addrun(), sizeof(sockaddr_un));
		if (nRet != 0)
		{
			return -6;
		}
	}

	m_status = 2;
	return 0;
}

int CLocalSocket::Send(const Buffer& data)
{
	if (m_status < 2 || m_socket == -1)
	{
		return-1;
	}

	ssize_t index = 0;
	while (index < (ssize_t)data.size())
	{
		ssize_t len = write(m_socket, (char*)data + index, data.size() - index);
		if (len == 0)
		{
			return -2;
		}
		if (len < 0)
		{
			return -3;
		}
		index += len;
	}

	return 0;
}

int CLocalSocket::Recv(Buffer& data)
{
	if (m_status < 2 || m_socket == -1)
	{
		return-1;
	}
	ssize_t len = read(m_socket, data, data.size());
	if (len > 0)
	{
		data.resize(len);
		return (int)len;//收到数据
	}
	if (len < 0)
	{
		if (errno == EINTR || errno == EAGAIN)
		{
			data.clear();
			return 0;//未收到数据
		}
		return -2;//发生错误
	}

	return -3;//套接字被关闭
}

int CLocalSocket::Close()
{
	return CSocketBase::Close();
}