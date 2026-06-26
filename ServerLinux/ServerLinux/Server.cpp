#include "Server.h"
#include "Logger.h"

CServer::CServer()
{
	m_server = NULL;
	m_business = NULL;
}

CServer::~CServer()
{
}

int CServer::Init(CBusiness* business, const Buffer& ip, short port)
{
	if (business == NULL)
	{
		return -1;
	}

	int nRet = 0;

	m_business = business;
	nRet = m_process.SetEntryFunc(&CBusiness::BusinessProc, m_business, &m_process);
	if (nRet != 0)
	{
		return -2;
	}

	nRet = m_process.CreateSubProc();
	if (nRet != 0)
	{
		return -3;
	}

	nRet = m_pool.Start(2);
	if (nRet != 0)
	{
		return -4;
	}

	nRet = m_epoll.Creat(2);
	if (nRet != 0)
	{
		return -5;
	}

	m_server = new CSocket();
	if (m_server == NULL)
	{
		return -6;
	}

	nRet = m_server->Init(CSockParam(ip, port, SOCK_ISSERVER | SOCK_ISNETWORK));
	if (nRet != 0)
	{
		return -7;
	}

	nRet = m_epoll.Add(*m_server, EpollData((void*)m_server));
	if (nRet != 0)
	{
		return -8;
	}

	for (size_t i = 0; i < m_pool.Size(); i++)
	{
		nRet = m_pool.AddTask(&CServer::ThreadFunc, this);
		if (nRet != 0)
		{
			return -9;
		}
	}

	return 0;
}

int CServer::Run()
{
	while (m_server)
	{
		usleep(10);
	}

	return 0;
}

int CServer::Close()
{
	if (m_server)
	{
		CSocketBase* sock = m_server;
		m_server = NULL;
		m_epoll.Delete(*sock);
		delete sock;
	}
	m_epoll.Close();
	m_process.nSendFD(-1);
	m_pool.Close();

	return 0;
}

int CServer::ThreadFunc()
{
	int nRet = 0;
	EPEvents events;

	while (m_epoll != -1 && m_server)
	{
		ssize_t nSize = m_epoll.WaitEvents(events);
		if(nSize > 0)
		{
			for (ssize_t i = 0; i < nSize; i++)
			{
				if (events[i].events & EPOLLERR)
				{
					break;
				}
				else if(events[i].events & EPOLLIN)
				{
					if (m_server)
					{
						CSocketBase* pClient = NULL;
						nRet = m_server->Link(&pClient);
						if (nRet != 0)
						{
							continue;
						}

						nRet = m_process.nSendSocket(*pClient, *pClient);
						if (nRet != 0)
						{
							char buf[128] = "";
							snprintf(buf, sizeof(buf), "send client fd %d failed!", (int)*pClient);
							TRACEE("%s", buf);
							continue;
						}
					}
				}
			}
		}
		else if (nSize < 0)
		{
			break;
		}
	}

	return 0;
}

CBusiness::CBusiness()
{
	m_recvCallback = NULL;
	m_connectedCallback = NULL;
}
