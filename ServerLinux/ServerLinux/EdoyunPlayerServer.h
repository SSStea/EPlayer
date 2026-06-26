#pragma once
#include "Server.h"
#include <map>
#include "Logger.h"
#include "Function.h"

#define RET_ERR(nRet, err)																\
		if(nRet != 0)																	\
		{																				\
			TRACEE("ret = %d errno = %d msg = [%s] ", nRet, errno, strerror(errno));	\
			return err;																	\
		}

#define CONTINUE_WARN(nRet)																\
		if(nRet != 0)																	\
		{																				\
			TRACEW("ret = %d errno = %d msg = [%s] ", nRet, errno, strerror(errno));	\
			continue;																	\
		}

class CEdoyunPlayerServer : public CBusiness
{
public:
	CEdoyunPlayerServer(unsigned count);
	~CEdoyunPlayerServer();

	virtual int BusinessProc(CProcess* proc);

private:
	CEpoll m_epoll;
	CThreadPool m_pool;
	std::map<int, CSocketBase*> m_mapClients;
	unsigned m_nCount;

	int ThreadFunc();

	int Connected(CSocketBase* pClient);
	int Received(CSocketBase* pClient, const Buffer& data);
};

