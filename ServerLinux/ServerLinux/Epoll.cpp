#include "Epoll.h"

CEpoll::CEpoll()
{
	m_epoll = -1;
}

CEpoll::~CEpoll()
{
	Close();
}

int CEpoll::Creat(unsigned count)
{
	if (m_epoll != -1)
	{
		return -1;
	}

	m_epoll = epoll_create(count);
	if (m_epoll == -1)
	{
		return -2;
	}

	return 0;
}

ssize_t CEpoll::WaitEvents(EPEvents& events, int timeout)
{
	if (m_epoll == -1)
	{
		return -1;
	}

	EPEvents evs(EVENT_SIZE);
	int nRet = epoll_wait(m_epoll, evs.data(), (int)evs.size(), timeout);
	if (nRet == -1)
	{
		if (errno == EINTR || errno == EAGAIN)
		{
			return 0;
		}
		return -2;
	}
	if (nRet > (int)events.size())
	{
		events.resize(nRet);
	}
	memcpy(events.data(), evs.data(), sizeof(epoll_event) * nRet);

	return nRet;
}

int CEpoll::Add(int nFd, const EpollData& data, uint32_t events)
{
	if (m_epoll == -1)
	{
		return -1;
	}

	epoll_event ev = { events, data };
	int nRet = epoll_ctl(m_epoll, EPOLL_CTL_ADD, nFd, &ev);
	if (nRet == -1)
	{
		return -2;
	}

	return 0;
}

int CEpoll::Modify(int nFd, uint32_t events, const EpollData& data)
{
	if (m_epoll == -1)
	{
		return -1;
	}

	epoll_event ev = { events, data };
	int nRet = epoll_ctl(m_epoll, EPOLL_CTL_MOD, nFd, &ev);
	if (nRet == -1)
	{
		return -2;
	}

	return 0;
}

int CEpoll::Delete(int nFd)
{
	if (m_epoll == -1)
	{
		return -1;
	}

	int nRet = epoll_ctl(m_epoll, EPOLL_CTL_DEL, nFd, NULL);
	if (nRet == -1)
	{
		return -2;
	}

	return 0;
}

void CEpoll::Close()
{
	if (m_epoll != -1)
	{
		int nFd = m_epoll;
		m_epoll = -1;
		close(nFd);
	}
}
