#include "ThreadPool.h"

CThreadPool::CThreadPool()
{
	m_server = NULL;
	timespec tp = { 0, 0 };
	clock_gettime(CLOCK_REALTIME, &tp);

	char* buf = NULL;
	asprintf(&buf, "%d.%d.sock", tp.tv_sec % 10000, tp.tv_nsec % 100000);
	if( buf != NULL )
	{
		m_path = buf;
		free(buf);
	}
	usleep(1);
}

CThreadPool::~CThreadPool()
{
	Close();
}

int CThreadPool::Start(unsigned count)
{
	if (m_server != NULL)
	{
		return -1;
	}

	if (m_path.size() == 0)
	{
		return -2;
	}

	m_server = new CSocket();
	if (m_server == NULL)
	{
		return -3;
	}
	int nRet = m_server->Init(CSockParam(m_path, SOCK_ISSERVER));
	if (nRet != 0)
	{
		return -4;
	}

	nRet = m_epoll.Creat(count);
	if (nRet != 0)
	{
		return -5;
	}
	nRet = m_epoll.Add(*m_server, EpollData((void*)m_server));
	if (nRet != 0)
	{
		return -6;
	}

	m_threads.resize(count);
	for (unsigned i = 0; i < count; i++)
	{
		m_threads[i] = new CThread(&CThreadPool::TaskDispatch, this);
		if (m_threads[i] == NULL)
		{
			return -7;
		}
		nRet = m_threads[i]->Start();
		if (nRet != 0)
		{
			return -8;
		}
	}

	return 0;
}

void CThreadPool::Close()
{
	m_epoll.Close();
	if (m_server)
	{
		CSocketBase* p = m_server;
		m_server = NULL;
		delete p;
	}
	for (auto& thread : m_threads)
	{
		if (thread)
		{
			delete thread;
		}
	}
	m_threads.clear();
	unlink(m_path);
}

size_t CThreadPool::Size() const
{
	return m_threads.size();
}

int CThreadPool::TaskDispatch()
{
	while (m_epoll != -1)
	{
		EPEvents events;
		int nRet = 0;
		ssize_t esize = m_epoll.WaitEvents(events);
		if (esize > 0)
		{
			for (ssize_t i = 0; i < esize; i++)
			{
				if (events[i].events & EPOLLIN)
				{
					CSocketBase* pClient = NULL;
					if (events[i].data.ptr == m_server)
					{
						nRet = m_server->Link(&pClient);
						if (nRet != 0)
						{
							continue;
						}

						nRet = m_epoll.Add(*pClient, EpollData((void*)pClient));
						if (nRet != 0)
						{
							delete pClient;
							continue;
						}
					}
					else
					{
						pClient = (CSocketBase*)events[i].data.ptr;
						if (pClient)
						{
							CFuncBase* base = NULL;
							Buffer data(sizeof(base));

							nRet = pClient->Recv(data);
							if (nRet <= 0)
							{
								m_epoll.Delete(*pClient);
								delete pClient;
								continue;
							}
							else
							{
								memcpy(&base, (char*)data, sizeof(base));
								if (base != NULL)
								{
									(*base)();
									delete base;
								}
							}
						}
					}
				}
			}
		}
	}

	return 0;
}
