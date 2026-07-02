#pragma once
#include "http_parser.h"
#include "Public.h"
#include <map>

class CHttpParser
{
public:
	CHttpParser();
	~CHttpParser();
	CHttpParser(const CHttpParser& http);
	CHttpParser& operator=(const CHttpParser& http);

public:
	size_t Parser(const Buffer& data);
	unsigned Method() const;//GET POST...
	const std::map<Buffer, Buffer>& Headers();
	const Buffer& Status() const;
	const Buffer& Url() const;
	const Buffer& Body() const;
	unsigned Errno() const;

protected:
	//这里的parser就是成员m_parser，在构造函数中m_parser = this，因此这些
	// 静态的回调函数就可以调用下面的成员方法
	static int OnMessageBegin(http_parser* parser);
	static int OnUrl(http_parser* parser, const char* at, size_t length);
	static int OnStatus(http_parser* parser, const char* at, size_t length);
	static int OnHeaderField(http_parser* parser, const char* at, size_t length);
	static int OnHeaderValue(http_parser* parser, const char* at, size_t length);
	static int OnHeadersComplete(http_parser* parser);
	static int OnBody(http_parser* parser, const char* at, size_t length);
	static int OnMessageComplete(http_parser* parser);

	int OnMessageBegin();
	int OnUrl(const char* at, size_t length);
	int OnStatus(const char* at, size_t length);
	int OnHeaderField(const char* at, size_t length);
	int OnHeaderValue(const char* at, size_t length);
	int OnHeadersComplete();
	int OnBody(const char* at, size_t length);
	int OnMessageComplete();

private:
	http_parser m_parser;
	http_parser_settings m_parser_setting;
	std::map<Buffer, Buffer> m_mapHeaderValues;
	Buffer m_status;
	Buffer m_url;
	Buffer m_body;
	bool m_bComplete;
	Buffer m_lastField;
};

class UrlParser
{
public:
	UrlParser(const Buffer& url);
	~UrlParser(){}
	int Parser();
	Buffer operator[](const Buffer& name);
	Buffer Protocol() const;
	Buffer Host() const;
	//默认返回80
	int Port() const;
	void SetUrl(const Buffer& url);
	Buffer RetUrlInfo() const;

private:
	Buffer m_url;
	Buffer m_protocol;
	Buffer m_host;
	Buffer m_urlInfo;
	int m_port;
	std::map<Buffer, Buffer> m_mapValues;
};