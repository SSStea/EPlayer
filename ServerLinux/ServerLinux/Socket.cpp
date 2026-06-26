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
	this->attr = attr | SOCK_ISNETWORK;
	addr_in.sin_family = AF_INET;
	addr_in.sin_port = htons(port);
	addr_in.sin_addr.s_addr = inet_addr(ip);
}

CSockParam::CSockParam(const Buffer& path, int arrt)
{
	ip = path;
	this->attr = arrt;
	addr_un.sun_family = AF_UNIX;
	strcpy(addr_un.sun_path, path);
}

CSockParam::CSockParam(const sockaddr_in* addr, int arrt)
{
	this->attr = attr | SOCK_ISNETWORK;
	memcpy(&addr_in, addr, sizeof(sockaddr_in));
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
		if ((m_param.attr & SOCK_ISSERVER) && //服务器
			((m_param.attr & SOCK_ISNETWORK) == 0))// 非IP协议
		{
			unlink(m_param.ip);
		}
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

CSocketBase::operator const sockaddr_in* () const
{
	return &m_param.addr_in;
}

CSocketBase::operator sockaddr_in* ()
{
	return &m_param.addr_in;
}

CSocket::CSocket():CSocketBase()
{
}

CSocket::CSocket(int sock) : CSocketBase()
{
	m_socket = sock;
}

CSocket::~CSocket()
{
	Close();
}

int CSocket::Init(const CSockParam& param)
{
	if (m_status != 0)
	{
		return -1;
	}

	m_param = param;
	int type = (m_param.attr & SOCK_ISUDP) ? SOCK_DGRAM : SOCK_STREAM;
	if(m_socket == -1)
	{
		if (param.attr & SOCK_ISNETWORK)
		{
			m_socket = socket(PF_INET, type, 0);
		}
		else
		{
			m_socket = socket(PF_LOCAL, type, 0);
		}
	}
	else
	{
		m_status = 2;//如果已经有socket那么这是accept来的套接字，应该置为已连接状态
	}
	if (m_socket == -1)
	{
		return -2;
	}

	int nRet = 0;
	if (m_param.attr & SOCK_ISSERVER)//判断是否是服务器
	{
		if (param.attr & SOCK_ISNETWORK)
		{
			nRet = bind(m_socket, m_param.addrin(), sizeof(sockaddr_in));
		}
		else
		{
			nRet = bind(m_socket, m_param.addrun(), sizeof(sockaddr_un));
		}
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

	if(m_status == 0)
	{
		m_status = 1;
	}
	return 0;
}

int CSocket::Link(CSocketBase** pClient)
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
		socklen_t len = 0;
		int nFd = -1;

		if (m_param.attr & SOCK_ISNETWORK)
		{
			param.attr |= SOCK_ISNETWORK;
			len = sizeof(sockaddr_in);
			nFd = accept(m_socket, param.addrin(), &len);
		}
		else
		{
			len = sizeof(sockaddr_un);
			nFd = accept(m_socket, param.addrun(), &len);
		}
		if (nFd == -1)
		{
			return -3;
		}

		*pClient = new CSocket(nFd);
		if (*pClient == NULL)
		{
			return -4;
		}
		nRet = (*pClient)->Init(param);
		if (nRet != 0)
		{
			printf("%s(%d) : [%s] ret = %d\n", __FILE__, __LINE__, __FUNCTION__, nRet);
			delete *pClient;
			*pClient = NULL;
			return -5;
		}
	}
	else
	{
		if (m_param.attr & SOCK_ISNETWORK)
		{
			nRet = connect(m_socket, m_param.addrin(), sizeof(sockaddr_in));
		}
		else
		{
			nRet = connect(m_socket, m_param.addrun(), sizeof(sockaddr_un));
		}
		if (nRet != 0)
		{
			printf("%d msg:%s\n", errno, strerror(errno));
			return -6;
		}
	}

	m_status = 2;
	return 0;
}

int CSocket::Send(const Buffer& data)
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

int CSocket::Recv(Buffer& data)
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

int CSocket::Close()
{
	return CSocketBase::Close();
}