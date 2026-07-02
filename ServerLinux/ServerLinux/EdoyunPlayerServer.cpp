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
	//连接在Server已经做完了，这里就打印客户端参数
	sockaddr_in* paddr = *pClient;
	
	TRACEI("client connected info: addr=%s, port=%d \n", inet_ntoa(paddr->sin_addr), paddr->sin_port);

	return 0;
}

int CEdoyunPlayerServer::Received(CSocketBase* pClient, const Buffer& data)
{
	int nRet = 0;

	nRet = HttpParser(data);
	if (nRet != 0)
	{
		char buf[128] = "";
		snprintf(buf, sizeof(buf), "http parser failed! %d", nRet);
		TRACEE("%s", buf);
	}

	Buffer response = MakeResponse(nRet);
	nRet = pClient->Send(response);
	if (nRet != 0)
	{
		char buf[128] = "";
		snprintf(buf, sizeof(buf), "http response failed! %d [%s]", nRet, (char*)response);
		TRACEE("%s", buf);
	}
	else
	{
		char buf[128] = "";
		snprintf(buf, sizeof(buf), "http response failed! %d", nRet);
		TRACEI("%s", buf);
	}

	return 0;
}

int CEdoyunPlayerServer::HttpParser(const Buffer& data)
{
	CHttpParser parser;

	size_t size = parser.Parser(data);
	if (size == 0 || (parser.Errno() != 0))
	{
		TRACEE("size: %llu errno: %d", size, parser.Errno());
		return -1;
	}

	if (parser.Method() == HTTP_GET)
	{
		//get处理
		UrlParser url("https://172.16.208.6" + parser.Url());
		int nRet = url.Parser();
		if (nRet != 0)
		{
			TRACEE("ret = %d, url[%s]", nRet, "https://172.16.208.6" + parser.Url());
			return -2;
		}

		Buffer urlInfo = url.RetUrlInfo();
		if (urlInfo == "Login")
		{
			Buffer time = url["time"];
			Buffer salt = url["salt"];
			Buffer user = url["user"];
			Buffer sign = url["sign"];
			TRACEI("time = %s, salt = %s, user = %s, sign = %s", (char*)time, (char*)salt, (char*)user, (char*)sign);

			edoyunLogin_user_mysql dbUser;
			Result res;
			Buffer sql = dbUser.Query("user_name=\"" + user + "\"");
			nRet = m_pDb->Exec(sql, res, dbUser);
			if (nRet != 0)
			{
				TRACEE("sql: %s, ret: %d", (char*)sql, nRet);
				return -3;
			}

			if (res.size() == 0)
			{
				TRACEE("No Result sql: %s, ret: %d", (char*)sql, nRet);
				return -4;
			}
			else if (res.size() != 1)
			{
				TRACEE("Result More Than One sql: %s, ret: %d", (char*)sql, nRet);
				return -5;
			}
			auto userData = res.front();
			Buffer pwd = userData->Fields["user_password"]->Value.String;
			TRACEI("password: %s", (char*)pwd);

			const char* MD5_KEY = "*&^%$#@b.v+h-b*g/h@n!h#n$d^ssx,.kl<kl";
			Buffer md5str = time + MD5_KEY + pwd + salt;
			Buffer md5_calc = CCrypto::MD5(md5str);
			TRACEI("Calc md5 = %s", (char*)md5_calc);
			if (md5_calc != sign)
			{
				return -6;
			}

			return 0;
		}
	}
	else if (parser.Method() == HTTP_POST)
	{
		//post处理
	}

	return -7;
}

Buffer CEdoyunPlayerServer::MakeResponse(int nRet)
{
	Json::Value root;
	root["status"] = nRet;
	if (nRet != 0)
	{
		root["message"] = "Login Fail, Username or Password error!";
	}
	else
	{
		root["message"] = "Login Success!";
	}

	Buffer json = root.toStyledString();
	Buffer res = "HTTP/1.1 200 OK\r\n";

	time_t t;
	time(&t);
	tm* ptm = localtime(&t);
	char temp[64] = "";
	strftime(temp, sizeof(temp), "%a, %d %b %G %T GMT", ptm);
	Buffer Date = (Buffer)"Date: " + temp;

	Buffer Server = "Server: Edoyun/1.0\r\nContent-Type: text/html; charset=utf-8\r\nX-Frame-Option: DENY\r\n";
	
	snprintf(temp, sizeof(temp), "%d", json.size());
	Buffer Length = Buffer("Content-Length: ") + temp + "\r\n";

	Buffer Stub = "X-Content-Type-Options: nosniff\r\nReferrer-Policy: same-origin\r\n\r\n";

	res += Date + Server + Length + Stub + json;
	TRACEI("response: %s", (char*)res);

	return res;
}