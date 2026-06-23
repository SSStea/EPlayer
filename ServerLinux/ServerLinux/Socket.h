#pragma once
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <fcntl.h>

class Buffer : public std::string
{
public:
	Buffer() : std::string() {}
	Buffer(size_t size);

	operator char* ();

	operator char* () const;

	operator const char* () const;
};

enum SockAttr{
	SOCK_ISSERVER = 1, //是否服务器，1表示是，0表示客户端
	SOCK_ISNONBLOCK = 2,   //是否非阻塞，1表示是，0表示否
	SOCK_ISUDP = 4,		//是否为UDP，1表示UDP，0表示TCP

};

class CSockParam
{
public:
	CSockParam();
	CSockParam(const Buffer& ip, short port, int attr);
	CSockParam(const Buffer& path, int arrt);

	~CSockParam(){}

	CSockParam(const CSockParam& param);

	CSockParam& operator=(const CSockParam& param);

	sockaddr* addrin();

	sockaddr* addrun();

public:
	//地址
	sockaddr_in addr_in;
	sockaddr_un addr_un;
	//ip、端口
	Buffer ip;
	short port;
	//参考SockAttr
	int attr;
};

class CSocketBase
{
public:
	CSocketBase();
	//传递析构操作
	virtual ~CSocketBase();

	//初始化 服务器 套接字创建、bind、listen； 客户端 套接字创建
	virtual int Init(const CSockParam& param) = 0;
	//链接   服务器 accept； 客户端 connect   对于UDP来讲可以忽略
	virtual int Link(CSocketBase** pClient = NULL) = 0;
	//收、发、关闭
	virtual int Send(const Buffer& data) = 0;
	virtual int Recv(Buffer& data) = 0;
	virtual int Close();

protected:
	//套接字描述符，默认-1
	int m_socket;
	//状态 0未初始化 1初始化完成 2连接完成 3关闭
	int m_status;
	CSockParam m_param;
};

class CLocalSocket : public CSocketBase
{
public:
	CLocalSocket();
	CLocalSocket(int sock);//客户端构造函数
	//传递析构操作
	virtual ~CLocalSocket();

	//初始化 服务器 套接字创建、bind、listen； 客户端 套接字创建
	virtual int Init(const CSockParam& param);
	//链接   服务器 accept； 客户端 connect   对于UDP来讲可以忽略
	virtual int Link(CSocketBase** pClient = NULL);
	//收、发、关闭
	virtual int Send(const Buffer& data);
	//接收数据 大于零，表示成功；小于零，表示失败；等于零，表示未收到但无错误
	virtual int Recv(Buffer& data);
	virtual int Close();
};

