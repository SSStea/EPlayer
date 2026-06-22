#pragma once
#include <unistd.h>
#include <sys/epoll.h>
#include <vector>
#include <errno.h>
#include <sys/signal.h>
#include <memory.h>

#define EVENT_SIZE 128

class EpollData
{
public:
	EpollData()
	{
		m_data.u64 = 0;
	}
	EpollData(void* ptr)
	{
		m_data.ptr = ptr;
	}
	explicit EpollData(int nFd)
	{
		m_data.fd = nFd;
	}
	explicit EpollData(uint32_t u32)
	{
		m_data.u32 = u32;
	}
	explicit EpollData(uint64_t u64)
	{
		m_data.u64 = u64;
	}
	EpollData(const EpollData& data)
	{
		m_data.u64 = data.m_data.u64;
	}

	operator epoll_data_t()
	{
		return m_data;
	}
	operator epoll_data_t() const
	{
		return m_data;
	}
	operator epoll_data_t*()
	{
		return &m_data;
	}
	operator const epoll_data_t* () const
	{
		return &m_data;
	}

	EpollData& operator=(const EpollData& data)
	{
		if (this != &data)
		{
			m_data.u64 = data.m_data.u64;
		}
		return *this;
	}
	EpollData& operator=(void* data)
	{
		m_data.ptr = data;
		return *this;
	}
	EpollData& operator=(int data)
	{
		m_data.fd = data;
		return *this;
	}
	EpollData& operator=(uint32_t data)
	{
		m_data.u32 = data;
		return *this;
	}
	EpollData& operator=(uint64_t data)
	{
		m_data.u64 = data;
		return *this;
	}

private:
	epoll_data_t m_data;//联合体
};

using EPEvents = std::vector<epoll_event>;

class CEpoll
{
public:
	CEpoll();
	~CEpoll();
public:
	CEpoll(const CEpoll&) = delete;
	operator int()const
	{
		return m_epoll;
	}
	int Creat(unsigned count);
	ssize_t WaitEvents(EPEvents& events, int timeout = 10);
	int Add(int nFd, const EpollData& data = EpollData((void*)0), uint32_t events = EPOLLIN);
	int Modify(int nFd, uint32_t events, const EpollData& data = EpollData((void*)0));
	int Delete(int nFd);
	void Close();

private:
	int m_epoll;
};

