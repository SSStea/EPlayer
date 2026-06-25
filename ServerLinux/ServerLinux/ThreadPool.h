#pragma once
#include "Epoll.h"
#include "Thread.h"
#include "Function.h"
#include "Socket.h"

class CThreadPool
{
public:
	CThreadPool();
	~CThreadPool();

	CThreadPool(const CThreadPool&) = delete;
	CThreadPool& operator=(const CThreadPool&) = delete;

public:
	int Start(unsigned count);
	void Close();
	template<typename _FUNCTION_, typename... _ARGS_>
	int AddTask(_FUNCTION_ func, _ARGS_... args);
	size_t Size() const;

private:
	CEpoll					m_epoll;
	std::vector<CThread*>	m_threads;
	CSocketBase*			m_server;
	Buffer					m_path;

	int TaskDispatch();
};

template<typename _FUNCTION_, typename ..._ARGS_>
inline int CThreadPool::AddTask(_FUNCTION_ func, _ARGS_ ...args)
{
	static thread_local CSocket client;//每个线程调用这个函数分配的客户端是不同的
	int nRet = 0;
	
	if (client == -1)
	{
		nRet = client.Init(CSockParam(m_path, 0));
		if (nRet != 0)
		{
			return -1;
		}
		nRet = client.Link();
		if (nRet != 0)
		{
			return -2;
		}
	}

	CFuncBase* base = new CFunction<_FUNCTION_, _ARGS_...>(func, args...);
	if (base == NULL)
	{
		return -3;
	}

	Buffer data(sizeof(base));
	memcpy(data, &base, sizeof(base));
	nRet = client.Send(data);
	if (nRet != 0)
	{
		delete base;
		return -4;
	}

	return 0;
}
