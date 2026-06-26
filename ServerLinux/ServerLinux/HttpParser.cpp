#include "HttpParser.h"

CHttpParser::CHttpParser()
{
    m_bComplete = false;

    memset(&m_parser, 0, sizeof(http_parser));
    m_parser.data = this;
    http_parser_init(&m_parser, HTTP_REQUEST);

    memset(&m_parser_setting, 0, sizeof(http_parser_settings));
    //注册回调函数
    m_parser_setting.on_message_begin = &CHttpParser::OnMessageBegin;
    m_parser_setting.on_url = &CHttpParser::OnUrl;
    m_parser_setting.on_status = &CHttpParser::OnStatus;
    m_parser_setting.on_header_field = &CHttpParser::OnHeaderField;
    m_parser_setting.on_header_value = &CHttpParser::OnHeaderValue;
    m_parser_setting.on_headers_complete = &CHttpParser::OnHeadersComplete;
    m_parser_setting.on_body = &CHttpParser::OnBody;
    m_parser_setting.on_message_complete = &CHttpParser::OnMessageComplete;
}

CHttpParser::~CHttpParser()
{
}

CHttpParser::CHttpParser(const CHttpParser& http)
{
    memcpy(&m_parser, &http.m_parser, sizeof(http_parser));
    m_parser.data = this;
	memcpy(&m_parser_setting, &http.m_parser_setting, sizeof(http_parser_settings));
	m_status = http.m_status;
    m_url = http.m_url;
    m_body = http.m_body;
    m_bComplete = http.m_bComplete;
    m_lastField = http.m_lastField;
}

CHttpParser& CHttpParser::operator=(const CHttpParser& http)
{
    if (this != &http)
    {
		memcpy(&m_parser, &http.m_parser, sizeof(http_parser));
		m_parser.data = this;
		memcpy(&m_parser_setting, &http.m_parser_setting, sizeof(http_parser_settings));
		m_status = http.m_status;
		m_url = http.m_url;
		m_body = http.m_body;
		m_bComplete = http.m_bComplete;
		m_lastField = http.m_lastField;
    }

    return *this;
}

size_t CHttpParser::Parser(const Buffer& data)
{
    m_bComplete = false;
    size_t nRet = http_parser_execute(&m_parser, &m_parser_setting, data, data.size());

    if (!m_bComplete)
    {
        m_parser.http_errno = 0x7F;
        return 0;
    }

    return nRet;
}

unsigned CHttpParser::Method() const
{
    return m_parser.method;
}

const std::map<Buffer, Buffer>& CHttpParser::Headers()
{
    return m_mapHeaderValues;
}

const Buffer& CHttpParser::Status() const
{
    return m_status;
}

const Buffer& CHttpParser::Url() const
{
    return m_url;
}

const Buffer& CHttpParser::Body() const
{
    return m_url;
}

unsigned CHttpParser::Errno() const
{
    return m_parser.http_errno;
}

int CHttpParser::OnMessageBegin(http_parser* parser)
{
    return ((CHttpParser*)parser->data)->OnMessageBegin();
}

int CHttpParser::OnUrl(http_parser* parser, const char* at, size_t length)
{
    return ((CHttpParser*)parser->data)->OnUrl(at, length);
}

int CHttpParser::OnStatus(http_parser* parser, const char* at, size_t length)
{
    return ((CHttpParser*)parser->data)->OnStatus(at, length);
}

int CHttpParser::OnHeaderField(http_parser* parser, const char* at, size_t length)
{
    return ((CHttpParser*)parser->data)->OnHeaderField(at, length);
}

int CHttpParser::OnHeaderValue(http_parser* parser, const char* at, size_t length)
{
    return ((CHttpParser*)parser->data)->OnHeaderValue(at, length);
}

int CHttpParser::OnHeadersComplete(http_parser* parser)
{
    return ((CHttpParser*)parser->data)->OnHeadersComplete();
}

int CHttpParser::OnBody(http_parser* parser, const char* at, size_t length)
{
    return ((CHttpParser*)parser->data)->OnBody(at, length);
}

int CHttpParser::OnMessageComplete(http_parser* parser)
{
    return ((CHttpParser*)parser->data)->OnMessageComplete();
}

int CHttpParser::OnMessageBegin()
{
    return 0;
}

int CHttpParser::OnUrl(const char* at, size_t length)
{
    m_url = Buffer(at, length);
    return 0;
}

int CHttpParser::OnStatus(const char* at, size_t length)
{
    m_status = Buffer(at, length);
    return 0;
}

int CHttpParser::OnHeaderField(const char* at, size_t length)
{
    m_lastField = Buffer(at, length);
    return 0;
}

int CHttpParser::OnHeaderValue(const char* at, size_t length)
{
    m_mapHeaderValues[m_lastField] = Buffer(at, length);
    return 0;
}

int CHttpParser::OnHeadersComplete()
{
    return 0;
}

int CHttpParser::OnBody(const char* at, size_t length)
{
    m_body = Buffer(at, length);
    return 0;
}

int CHttpParser::OnMessageComplete()
{
    m_bComplete = true;
    return 0;
}

UrlParser::UrlParser(const Buffer& url)
{
    m_url = url;
}

int UrlParser::Parser()
{
    //解析协议 url_info
    const char* pos = m_url;
    const char* target = strstr(pos, "://");
    if (target == NULL)
    {
        return -1;
    }
    m_protocol = Buffer(pos, target);

    //解析域名和端口
    pos = target + 3;
    target = strchr(pos, '/');
	if (target == NULL)
	{
        if (m_protocol.size() + 3 >= m_url.size())
        {
            return -2;
        }
        m_host = pos;
		return 0;
	}
    Buffer value = Buffer(pos, target);
	if (value.size() == 0)
	{
		return -3;
	}
    target = strchr(value, ':');
    if (target != NULL)
    {
        m_host = Buffer(pos, target);
        m_port = atoi(Buffer(target + 1, pos + value.size()));
    }
    else
    {
        m_host = value;
    }

    //解析url_info 
    pos = strchr(pos, '/');
    target = strchr(pos, '?');
    if (target == NULL)
    {
        m_urlInfo = pos;
        return 0;
    }
    else
    {
        m_urlInfo = Buffer(pos, target);

        //解析键值对key 和 value
        pos = target + 1;
        const char* t = NULL;
        do 
        {
            target = strchr(pos, '&');
            if (target == NULL)
            {
                t = strchr(pos, '=');
                if (t == NULL)
                {
                    return -4;
                }
				m_mapValues[Buffer(pos, t)] = Buffer(t + 1);
				pos = target + 1;
            }
            else
            {
                Buffer key_value(pos, target);
                t = strchr(key_value, '=');
                if (t == NULL)
                {
                    return -5;
                }
                m_mapValues[Buffer(key_value, t)] = Buffer(t + 1, key_value + key_value.size());
                pos = target + 1;
            }
        } while (target != NULL);
    }


    return 0;
}

Buffer UrlParser::operator[](const Buffer& name)
{
    auto it = m_mapValues.find(name);
    if (it == m_mapValues.end())
    {
        return Buffer();
    }
    return it->second;
}

Buffer UrlParser::Protocol() const
{
    return m_protocol;
}

Buffer UrlParser::Host() const
{
    return m_host;
}

int UrlParser::Port() const
{
    return m_port;
}

void UrlParser::SetUrl(const Buffer& url)
{
    m_url = url;
    m_protocol = "";
    m_host = "";
    m_port = 80;
    m_mapValues.clear();
}
