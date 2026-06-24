#pragma once
#include "Thread.h"
#include "Epoll.h"
#include "Socket.h"
#include <map>
#include <sys/timeb.h>
#include <stdarg.h>
#include <sstream>
#include <sys/stat.h>

enum LogLevel {
	LOG_INFO,
	LOG_DEBUG,
	LOG_WARNING,
	LOG_ERROR,
	LOG_FATAL
};

class LogInfo;

class CLoggerServer
{
public:
	CLoggerServer();
	~CLoggerServer();

	CLoggerServer(const CLoggerServer&) = delete;
	CLoggerServer& operator=(const CLoggerServer&) = delete;

public:
	int Start();
	int Close();
	static void Trace(const LogInfo& info);//给其他非日志进程的进程和线程使用的
	static Buffer GetTimeStr();

private:
	CThread m_Thread;
	CEpoll m_epoll;
	CSocketBase* m_server;
	Buffer m_path;
	FILE* m_file;

	void WriteLog(const Buffer& data);
	int ThreadFunc();
};

class LogInfo
{
public:
	LogInfo(
		const char* file, 
		int line, 
		const char* func, 
		pid_t pid, 
		pthread_t tid, 
		int level, 
		const char* fmt,...
	);//Trace调用

	LogInfo(
		const char* file,
		int line,
		const char* func,
		pid_t pid,
		pthread_t tid,
		int level
	);//自己主动发

	LogInfo(
		const char* file,
		int line,
		const char* func,
		pid_t pid,
		pthread_t tid,
		int level,
		const void* pData,
		size_t nSize
	);//Trace调用

	~LogInfo();

	operator Buffer() const;

	template<typename T> LogInfo& operator<<(const T& data);

private:
	bool bAuto;//默认是false，流式日志则为true
	Buffer m_buf;
};

template<typename T>
inline LogInfo& LogInfo::operator<<(const T& data)
{
	std::stringstream stream;
	stream << data;
	m_buf += stream.str();

	return *this;
}

#ifndef TRACE
#define TRACEI(...) CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_INFO, __VA_ARGS__))
#define TRACED(...) CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_DEBUG, __VA_ARGS__))
#define TRACEW(...) CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_WARNING, __VA_ARGS__))
#define TRACEE(...) CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_ERROR, __VA_ARGS__))
#define TRACEF(...) CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_FATAL, __VA_ARGS__))

//LOGI<<"hello"<<"how are you";
#define LOGI LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_INFO)
#define LOGD LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_DEBUG)
#define LOGW LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_WARNING)
#define LOGE LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_ERROR)
#define LOGF LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_FATAL)

//内存导出，每16字符一行：00 01 02 65.... ; ... a ...
#define DUMPI(data, size) CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_INFO, (const void*)data, size))
#define DUMPD(data, size) CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_DEBUG, (const void*)data, size))
#define DUMPW(data, size) CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_WARNING, (const void*)data, size))
#define DUMPE(data, size) CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_ERROR, (const void*)data, size))
#define DUMPF(data, size) CLoggerServer::Trace(LogInfo(__FILE__, __LINE__, __FUNCTION__, getpid(), pthread_self(), LOG_FATAL, (const void*)data, size))

#endif // !TRACE
