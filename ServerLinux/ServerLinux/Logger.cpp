#include "Logger.h"
#include <sys/stat.h>

CLoggerServer::CLoggerServer() : m_Thread(&CLoggerServer::ThreadFunc, this)
{
	m_server = NULL;
	char currPath[256] = "";
	getcwd(currPath, sizeof(currPath));
	m_path = currPath;
	m_path += "/log/" + GetTimeStr() +".log";
	printf("%s(%d) : [%s] path = %s \n", __FILE__, __LINE__, __FUNCTION__, (char*)m_path);
}

CLoggerServer::~CLoggerServer()
{
	Close();
}

int CLoggerServer::Start()
{
	if (m_server != NULL)
	{
		return -1;
	}

	if (access("log", W_OK | R_OK) != 0)
	{
		mkdir("log", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	}
	m_file = fopen(m_path, "w+");
	if (m_file == NULL)
	{
		return -2;
	}

	int nRet = m_epoll.Creat(1);
	if (nRet != 0)
	{
		return -3;
	}

	m_server = new CLocalSocket();
	if (m_server == NULL)
	{
		Close();
		return -4;
	}
	CSockParam param("./log/server.sock", (int)SOCK_ISSERVER);
	nRet = m_server->Init(param);
	if (nRet != 0)
	{
		Close();
		return -5;
	}

	nRet = m_epoll.Add(*m_server, EpollData((void*)m_server), EPOLLIN | EPOLLERR);
	if (nRet != 0)
	{
		Close();
		return -6;
	}

	nRet = m_Thread.Start();
	if (nRet != 0)
	{
		printf("%s(%d) <%s> pid = %d, errno = %d, msg: %s ret = %d\n",
			__FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), nRet);
		Close();
		return -7;
	}

	return 0;
}

int CLoggerServer::ThreadFunc()
{
	printf("%s(%d) : [%s] %d %d %p\n", __FILE__, __LINE__, __FUNCTION__, m_Thread.isValid(), (int)m_epoll, m_server);
	EPEvents events;
	std::map<int, CSocketBase*> mapClients;
	while (m_Thread.isValid() && m_epoll != -1 && m_server != NULL)
	{
		ssize_t ret = m_epoll.WaitEvents(events, 1);
		if (ret < 0)
		{
			break;
		}
		else if(ret > 0)
		{
			ssize_t i = 0;
			for (; i < ret; i++)
			{
				if (events[i].events & EPOLLERR)
				{
					break;
				}
				else if (events[i].events & EPOLLIN)
				{
					if (events[i].data.ptr == m_server)
					{
						CSocketBase* pClient = NULL;
						int nRet = m_server->Link(&pClient);
						printf("%s(%d) : [%s] ret = %d\n", __FILE__, __LINE__, __FUNCTION__, nRet);
						if (nRet < 0)
						{
							continue;
						}

						nRet = m_epoll.Add(*pClient, EpollData((void*)pClient), EPOLLIN | EPOLLERR);
						printf("%s(%d) : [%s] ret = %d\n", __FILE__, __LINE__, __FUNCTION__, nRet);
						if (nRet < 0)
						{
							delete pClient;
						}

						auto it = mapClients.find(*pClient);
						if (it->second != NULL)
						{
							delete it->second;
						}
						mapClients[*pClient] = pClient;
					}
					else
					{
						CSocketBase* pClient = (CSocketBase*)events[i].data.ptr;
						if (pClient != NULL)
						{
							Buffer data( 1024 * 1024 );
							int nRet = pClient->Recv(data);
							printf("%s(%d) : [%s] ret = %d\n", __FILE__, __LINE__, __FUNCTION__, nRet);
							if (nRet <= 0)
							{
								delete pClient;
								mapClients[*pClient] = NULL;
							}
							else
							{
								printf("%s(%d) : [%s] ret = %s\n", __FILE__, __LINE__, __FUNCTION__, (char*)data);
								WriteLog(data);
							}
						}
					}
				}
			}

			if (i != ret)
			{
				break;
			}
		}
	}

	for (auto it = mapClients.begin(); it != mapClients.end(); it++)
	{
		if (it->second)
		{
			delete it->second;
		}
	}
	mapClients.clear();

	return 0;
}

int CLoggerServer::Close()
{
	if (m_server != NULL)
	{
		CSocketBase* p = m_server;
		m_server = NULL;
		delete p;
	}
	m_epoll.Close();
	m_Thread.Stop();
	return 0;
}

void CLoggerServer::Trace(const LogInfo& info)
{
	static thread_local CLocalSocket client;//每次线程进Trace都会调用构造函数创建一个新的客户端
	int nRet = 0;
	if (client == -1)
	{
		nRet = client.Init(CSockParam("./log/server.sock", 0));
		if (nRet != 0)
		{
#ifdef _DEBUG
			printf("%s(%d) : [%s] ret = %d \n", __FILE__, __LINE__, __FUNCTION__, nRet);
#endif // _DEBUG

			return;
		}
		printf("%s(%d) : [%s] ret = %d client = %d\n", __FILE__, __LINE__, __FUNCTION__, nRet, (int)client);
		nRet = client.Link();
		printf("%s(%d) : [%s] ret = %d client = %d\n", __FILE__, __LINE__, __FUNCTION__, nRet, (int)client);
	}
	nRet = client.Send(info);
	printf("%s(%d) : [%s] ret = %d client = %d\n", __FILE__, __LINE__, __FUNCTION__, nRet, (int)client);
}

Buffer CLoggerServer::GetTimeStr()
{
	Buffer result(1024);
	timeb tmb;
	ftime(&tmb);
	tm* pTm = localtime(&tmb.time);
	int nSize = snprintf(result, result.size(),
		"%04d-%02d-%02d_%02d-%02d-%02d.%03d",
		pTm->tm_year + 1900,
		pTm->tm_mon + 1,
		pTm->tm_mday,
		pTm->tm_hour,
		pTm->tm_min,
		pTm->tm_sec,
		tmb.millitm
	);
	result.resize(nSize);

	return result;
}

void CLoggerServer::WriteLog(const Buffer& data)
{
	if(m_file != NULL)
	{
		FILE* pFile = m_file;
		fwrite((char*)data, 1, data.size(), pFile);
		fflush(pFile);

#ifdef _DEBUG
		printf("%s", (char*)data);
#endif // _DEBUG

	}
}

//Trace调用 TRACE
LogInfo::LogInfo(
	const char* file,
	int line,
	const char* func,
	pid_t pid,
	pthread_t tid,
	int level,
	const char* fmt, ...
)
{
	const char sLevel[][8] = {
		"INFO", "DEBUG", "WARNING", "ERROR", "FATAL"
	};
	char* buf = NULL;
	bAuto = false;

	int count = asprintf(&buf, "%s(%d):[%s][%s]<%d-%d>(%s) ",
		file, line, sLevel[level], (char*)CLoggerServer::GetTimeStr(), pid, tid, func);
	if (count > 0)
	{
		m_buf = buf;
		free(buf);
	}
	else
	{
		return;
	}

	va_list ap;
	va_start(ap, fmt);

	count = vasprintf(&buf, fmt, ap);
	if (count > 0)
	{
		m_buf += buf;
		free(buf);
	}
	m_buf += "\n";

	va_end(ap);
}

//自己主动发 LOG
LogInfo::LogInfo(
	const char* file,
	int line,
	const char* func,
	pid_t pid,
	pthread_t tid,
	int level)
{
	const char sLevel[][8] = {
		"INFO", "DEBUG", "WARNING", "ERROR", "FATAL"
	};
	char* buf = NULL;
	bAuto = true;

	int count = asprintf(&buf, "%s(%d):[%s][%s]<%d-%d>(%s) ",
		file, line, sLevel[level], (char*)CLoggerServer::GetTimeStr(), pid, tid, func);
	if (count > 0)
	{
		m_buf = buf;
		free(buf);
	}
}

//Trace调用 DUMP
LogInfo::LogInfo(
	const char* file,
	int line,
	const char* func,
	pid_t pid,
	pthread_t tid,
	int level,
	const void* pData,
	size_t nSize
)
{
	const char sLevel[][8] = {
		"INFO", "DEBUG", "WARNING", "ERROR", "FATAL"
	};
	char* buf = NULL;
	bAuto = false;

	int count = asprintf(&buf, "%s(%d):[%s][%s]<%d-%d>(%s) \n",
		file, line, sLevel[level], (char*)CLoggerServer::GetTimeStr(), pid, tid, func);
	if (count > 0)
	{
		m_buf = buf;
		free(buf);
	}
	else
	{
		return;
	}

	Buffer out;
	size_t i = 0;
	char* Data = (char*)pData;
	for (; i < nSize; i++)
	{
		char buf[16] = "";
		snprintf(buf, sizeof(buf), "%02X ", Data[i] & 0xFF);//显示16进制
		m_buf += buf;
		if (0 == (i + 1) % 16)
		{
			m_buf += "\t; ";
			for (size_t j = i - 15; j <= i; j++)
			{
				if (((Data[j] & 0xFF) > 31) && ((Data[j] & 0xFF) < 0x7F))
				{//ASCII码可显示的字符，ASCII码不会显示中文
					m_buf += Data[j];
				}
				else
				{
					m_buf += '.';
				}
			}
			m_buf += "\n";
		}
	}
	size_t k = i % 16;//处理最后不够16字节的尾巴
	if (k != 0)
	{
		for (size_t j = 0; j < 16 - k; j++)
		{
			m_buf += "   ";
		}

		m_buf += "\t; ";
		for (size_t j = i - k; j <= i; j++)
		{
			if (((Data[j] & 0xFF) > 31) && ((Data[j] & 0xFF) < 0x7F))
			{//ASCII码可显示的字符
				m_buf += Data[j];
			}
			else
			{
				m_buf += '.';
			}
		}
		m_buf += "\n";
	}
}

LogInfo::~LogInfo()
{
	if (bAuto)
	{
		m_buf += "\n";
		CLoggerServer::Trace(*this);
	}
}

LogInfo::operator Buffer() const
{
	return m_buf;
}
