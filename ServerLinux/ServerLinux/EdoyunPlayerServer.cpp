#include "EdoyunPlayerServer.h"

CEdoyunPlayerServer::CEdoyunPlayerServer(unsigned count) : CBusiness()
{
	m_nCount = count;
}

CEdoyunPlayerServer::~CEdoyunPlayerServer()
{
	m_epoll.Close();
	m_pool.Close();
	for (auto& client : m_mapClients)
	{
		if (client.second)
		{
			delete client.second;
		}
	}
	m_mapClients.clear();
}

int CEdoyunPlayerServer::BusinessProc(CProcess* proc)
{
	int nRet = 0;
	int nSock = 0;
	sockaddr_in addr;

	nRet = m_epoll.Creat(m_nCount);
	RET_ERR(nRet, -1);

	nRet = m_pool.Start(m_nCount);
	RET_ERR(nRet, -2);

	for (unsigned i = 0; i < m_nCount; i++)
	{
		nRet = m_pool.AddTask(&CEdoyunPlayerServer::ThreadFunc, this);
		RET_ERR(nRet, -3);
	}

	while (m_epoll != -1)
	{
		nRet = proc->nRecvFd(nSock);
		if (nRet < 0 || nSock == 0)
		{
			break;
		}

		CSocketBase* pClient = new CSocket(nSock);
		if (pClient == NULL)
		{
			continue;
		}
		nRet = pClient->Init(CSockParam(&addr, SOCK_ISNETWORK));
		CONTINUE_WARN(nRet);

		nRet = m_epoll.Add(nSock, EpollData((void*)pClient));
		CONTINUE_WARN(nRet);
		if (m_connectedCallback)
		{
			(*m_connectedCallback)();
		}
	}

	return 0;
}

int CEdoyunPlayerServer::ThreadFunc()
{
	int nRet = 0;
	EPEvents events;

	while (m_epoll != -1)
	{
		ssize_t nSize = m_epoll.WaitEvents(events);
		if (nSize > 0)
		{
			for (ssize_t i = 0; i < nSize; i++)
			{
				if (events[i].events & EPOLLERR)
				{
					break;
				}
				else if (events[i].events & EPOLLIN)
				{
					CSocketBase* pClient = (CSocketBase*)events[i].data.ptr;
					if (pClient)
					{
						Buffer data;
						nRet = pClient->Recv(data);
						CONTINUE_WARN(nRet);
						if (m_recvCallback)
						{
							(*m_recvCallback)();
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
