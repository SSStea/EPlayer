#pragma once
#include <pthread.h>
#include <errno.h>
#include <map>
#include "Function.h"

class CThread
{
public:
	CThread();
	template<typename _FUNCTION_, typename... _ARGS_>
	CThread(_FUNCTION_ func, _ARGS_... args);

	~CThread() {}

	CThread(const CThread&) = delete;
	CThread operator=(const CThread&) = delete;

public:
	template<typename _FUNCTION_, typename... _ARGS_>
	int SetThreadFunc(_FUNCTION_ func, _ARGS_... args);
	int Start();
	int Pause();
	int Stop();
	bool isValid() const;

private:
	static void* ThreadEntry(void* arg);//__stdcall
	void EnterThread();//__thiscall
	static void Sigaction(int signo, siginfo_t* info, void* context);

private:
	CFuncBase* m_pFunc;
	pthread_t m_thread;
	bool m_bPaused;//1 表示暂停；0 表示运行中
	static std::map<pthread_t, CThread*> m_mapThread;
};
