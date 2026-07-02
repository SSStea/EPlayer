#include "EdoyunPlayerServer.h"

CEdoyunPlayerServer::CEdoyunPlayerServer(unsigned count) : CBusiness()
{
	m_nCount = count;
}

CEdoyunPlayerServer::~CEdoyunPlayerServer()
{
	if (m_pDb)
	{
		CDatabaseClient* pDb = m_pDb;
		m_pDb = NULL;
		pDb->Close();
		delete pDb;
	}

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
	using namespace std::placeholders;

	int nRet = 0;
	int nSock = 0;
	sockaddr_in addr;

	m_pDb = new CMySqlClient();
	if (m_pDb == NULL)
	{
		TRACEE("No More Memory");
		return -1;
	}
	KeyValue args;
	args["host"] = "172.16.208.100";
	args["user"] = "wang";
	args["password"] = "123456";
	args["port"] = "3306";
	args["db"] = "edoyun";
	nRet = m_pDb->Connect(args);
	RET_ERR(nRet, -2);

	edoyunLogin_user_mysql user;
	nRet = m_pDb->Exec(user.Create());
	RET_ERR(nRet, -3);

	//_1 _2是占位符，告诉模板函数可变参数有多少个我还不清楚，但是我先把位置占上
	nRet = setConnectedCallback(&CEdoyunPlayerServer::Connected, this, _1);
	RET_ERR(nRet, -4);

	nRet = recvCallback(&CEdoyunPlayerServer::Received, this, _1, _2);
	RET_ERR(nRet, -5);

	nRet = m_epoll.Creat(m_nCount);
	RET_ERR(nRet, -6);

	nRet = m_pool.Start(m_nCount);
	RET_ERR(nRet, -7);

	for (unsigned i = 0; i < m_nCount; i++)
	{
		nRet = m_pool.AddTask(&CEdoyunPlayerServer::ThreadFunc, this);
		RET_ERR(nRet, -8);
	}

	while (m_epoll != -1)
	{
		nRet = proc->nRecvSocket(nSock, &addr);
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

int CEdoyunPlayerServer::Connected(CSocketBase* pClient)
{
	return 0;
}

int CEdoyunPlayerServer::Received(CSocketBase* pClient, const Buffer& data)
{
	return 0;
}