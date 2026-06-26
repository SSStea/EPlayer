#pragma once
#include "Socket.h"
#include "ThreadPool.h"
#include "Epoll.h"
#include "Process.h"
#include "Function.h"

template<typename _FUNCTION_, typename... _ARGS_>
class CConnectedFunction : public CFuncBase
{
public:
	CConnectedFunction(_FUNCTION_ func, _ARGS_... args)
		: m_binder(std::forward<_FUNCTION_>(func), std::forward<_ARGS_>(args)...)
	{

	}

	virtual ~CConnectedFunction() {}

	virtual int operator()(CSocketBase* pClient)
	{
		return m_binder(pClient);
	}

	typename std::_Bindres_helper<int, _FUNCTION_, _ARGS_...>::type m_binder;
};

template<typename _FUNCTION_, typename... _ARGS_>
class CReceivedFunction : public CFuncBase
{
public:
	CReceivedFunction(_FUNCTION_ func, _ARGS_... args)
		: m_binder(std::forward<_FUNCTION_>(func), std::forward<_ARGS_>(args)...)
	{

	}

	virtual ~CReceivedFunction() {}

	virtual int operator()(CSocketBase* pClient, const Buffer& data)
	{
		return m_binder(pClient, data);
	}

	typename std::_Bindres_helper<int, _FUNCTION_, _ARGS_...>::type m_binder;
};

class CBusiness
{
public:
	CBusiness();

	virtual int BusinessProc(CProcess* proc) = 0;

	template<typename _FUNCTION_, typename... _ARGS_>
	int setConnectedCallback(_FUNCTION_ func, _ARGS_... args);

	template<typename _FUNCTION_, typename... _ARGS_>
	int recvCallback(_FUNCTION_ func, _ARGS_... args);

protected:
	CFuncBase* m_connectedCallback;
	CFuncBase* m_recvCallback;
};

class CServer
{
public:
	CServer();
	~CServer();

	CServer(const CServer&) = delete;
	CServer& operator=(const CServer&) = delete;

public:
	int Init(CBusiness* business, const Buffer& ip = "127.0.0.1", short port = 9527);
	int Run();
	int Close();

private:
	CThreadPool m_pool;
	CSocketBase* m_server;
	CEpoll m_epoll;
	CProcess m_process;
	CBusiness* m_business;//业务模块，需要手动释放

	int ThreadFunc();
};

template<typename _FUNCTION_, typename ..._ARGS_>
inline int CBusiness::setConnectedCallback(_FUNCTION_ func, _ARGS_ ...args)
{
	m_connectedCallback = new CFunction<_FUNCTION_, _ARGS_...>(func, args...);
	if (m_connectedCallback == NULL)
	{
		return -1;
	}

	return 0;
}

template<typename _FUNCTION_, typename ..._ARGS_>
inline int CBusiness::recvCallback(_FUNCTION_ func, _ARGS_ ...args)
{
	m_recvCallback = new CFunction<_FUNCTION_, _ARGS_...>(func, args...);
	if (m_recvCallback == NULL)
	{
		return -1;
	}

	return 0;
}
