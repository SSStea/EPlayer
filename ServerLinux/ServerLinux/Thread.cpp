#include "Thread.h"

std::map<pthread_t, CThread*> CThread::m_mapThread;

CThread::CThread()
{
	m_pFunc = NULL;
	m_thread = 0;
	m_bPaused = false;
}

template<typename _FUNCTION_, typename ..._ARGS_>
inline CThread::CThread(_FUNCTION_ func, _ARGS_ ...args)
	: m_pFunc(new CFunction< _FUNCTION_, _ARGS_...>(func, args...))
{
	m_thread = 0;
	m_bPaused = false;
}

template<typename _FUNCTION_, typename ..._ARGS_>
int CThread::SetThreadFunc(_FUNCTION_ func, _ARGS_ ...args)
{
	m_pFunc = new CFunction< _FUNCTION_, _ARGS_...>(func, args...);
	return 0;
}

int CThread::Start()
{
	int nRet = 0;
	pthread_attr_t attr;//线程属性
	nRet = pthread_attr_init(&attr);
	if (nRet != 0)
	{
		return -1;
	}
	//设置状态，当创建的线程结束exit之后，线程的状态是JOINABLE的，主线程可以jion操作结束线程
	nRet = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	if (nRet != 0)
	{
		return -2;
	}
	//设置竞争范围再本进程内部，不在全局内竞争
	nRet = pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
	if (nRet != 0)
	{
		return -3;
	}

	nRet = pthread_create(&m_thread, &attr, CThread::ThreadEntry, this);
	if (nRet != 0)
	{
		return -4;
	}
	m_mapThread[m_thread] = this;

	nRet = pthread_attr_destroy(&attr);
	if (nRet != 0)
	{
		return -5;
	}
	return 0;
}

int CThread::Pause()
{
	if (m_thread == 0)
	{
		return -1;
	}
	if (m_bPaused)
	{
		m_bPaused = false;
		return 0;
	}
	m_bPaused = true;
	int nRet = pthread_kill(m_thread, SIGUSR1);//给m_thread线程发送SIGUSR1信号量
	if (nRet != 0)
	{
		m_bPaused = false;
		return -2;
	}

	return 0;
}

int CThread::Stop()
{
	if (m_thread != 0)
	{
		pthread_t thread = m_thread;
		m_thread = 0;
		timespec ts;
		ts.tv_sec = 0;
		ts.tv_nsec = 100 * 1000000;//100ms
		int nRet = pthread_timedjoin_np(thread, NULL, &ts);//jion线程，等待100ms
		if (nRet == ETIMEDOUT)//如果超时未完成
		{
			pthread_detach(thread);//detach线程
			pthread_kill(thread, SIGUSR2);//发送一个SIGUSR2信号量
		}
	}
	return 0;
}

bool CThread::isValid() const
{
	return m_thread == 0;
}

void* CThread::ThreadEntry(void* arg)
{
	CThread* thiz = (CThread*)arg;

	struct sigaction act = { 0 };//注册信号量action
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = &CThread::Sigaction;//信号量action的响应函数为Sigaction
	sigaction(SIGUSR1, &act, NULL);//注册SIGUSR1信号
	sigaction(SIGUSR2, &act, NULL);//注册SIGUSR2信号

	thiz->EnterThread();

	if (thiz->m_thread != 0)
	{
		thiz->m_thread = 0;
	}
	pthread_t thread = pthread_self();//不是冗余，有可能stop函数清零m_thread
	auto it = m_mapThread.find(thread);
	if (it != m_mapThread.end())
	{
		m_mapThread[thread] = NULL;
	}
	pthread_detach(thread);
	pthread_exit(NULL);
}

void CThread::EnterThread()
{
	if (m_pFunc != NULL)
	{
		int nRet = (*m_pFunc)();
		if (nRet != 0)
		{
			printf("%s(%d):[%s] ret = %d", __FILE__, __LINE__, __FUNCTION__, nRet);
		}
	}
}

void CThread::Sigaction(int signo, siginfo_t* info, void* context)
{
	if (signo == SIGUSR1)
	{
		//留给暂停用
		pthread_t thread = pthread_self();
		auto it = m_mapThread.find(thread);
		if (it != m_mapThread.end())
		{
			if (it->second)
			{
				while (it->second->m_bPaused)
				{
					if (it->second->m_thread == 0)
					{
						pthread_exit(NULL);
						break;
					}
					usleep(1000);//1ms
				}
			}
		}
	}

	else if (signo == SIGUSR2)
	{
		//线程退出
		pthread_exit(NULL);
	}
}
