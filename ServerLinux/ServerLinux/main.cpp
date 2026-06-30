#include "Process.h"
#include "Logger.h"
#include "ThreadPool.h"
#include "EdoyunPlayerServer.h"
#include "HttpParser.h"

/*
 * 这个文件是服务端程序的启动入口。
 *
 * 当前设计思路：
 * 1. CProcess 表示“一个要被启动的子进程”。
 * 2. 每个 CProcess 先保存一个入口函数，比如 CreateLogServer 或 CreateClientServer。
 * 3. CreateSubProc() 调用 fork() 创建子进程。
 * 4. 子进程执行前面保存好的入口函数，父进程只记录子进程 pid 后继续往下启动其他服务。
 *
 * 这样写的好处是：
 * - main() 只负责描述“要启动哪些服务”，不会把每个服务的启动细节混在一起。
 * - CProcess 不需要知道具体入口函数叫什么、带几个参数；设计上由模板把函数和参数统一包装成 int()。
 * - 后续如果新增服务进程，只需要写一个 int XxxServer(CProcess* proc) 入口函数，
 *   然后在 main() 里 SetEntryFunc() + CreateSubProc() 即可。
 */



int CreateLogServer(CProcess* proc)
{
    //printf("%s(%d) <%s> pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
    CLoggerServer logServer;
    int nRet = logServer.Start();
    if (nRet != 0)
    {
        printf("%s(%d) <%s> pid = %d, errno = %d, msg: %s ret = %d\n",
            __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), nRet);
    }

    int nFd = 0;
    while (true)
    {
        nRet = proc->nRecvFd(nFd);
		printf("%s(%d) <%s> nFd = %d nRet = %d\n", __FILE__, __LINE__, __FUNCTION__, nFd, nRet);
        if (nFd <= 0)
        {
            break;
        }
    }

    nRet = logServer.Close();
    printf("%s(%d) <%s> nRet = %d\n", __FILE__, __LINE__, __FUNCTION__, nRet);

    return 0;
}

int CreateClientServer(CProcess* proc)
{
	//printf("%s(%d) <%s> pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());

    int nFd = -1;
	int nRet = proc->nRecvFd(nFd);
	//printf("%s(%d) <%s> nRet = %d\n", __FILE__, __LINE__, __FUNCTION__, nRet);
	if (nRet != 0)
	{
		//printf("%d msg:%s", errno, strerror(errno));
	}
	//printf("%s(%d) <%s> fd = %d\n", __FILE__, __LINE__, __FUNCTION__, nFd);
    
    sleep(1);
    char buf[10] = "";
    lseek(nFd, 0, SEEK_SET);
	read(nFd, buf, sizeof(buf));
	//printf("%s(%d) <%s> buf = %s\n", __FILE__, __LINE__, __FUNCTION__, buf);
    close(nFd);

    return 0;
}

int LogTest()
{
    char buffer[] = "hello edoyun! 冯老师";
    usleep(1000 * 100);
    TRACEI("here is log %d %c %f %g %s 哈哈 嘻嘻 易道云", 10, 'A', 1.0f, 2.0, buffer);
    DUMPD(buffer, sizeof(buffer));
    LOGE << 100 << " " << 'S' << " " << 0.12345f << " " << 1.23456789 << " " << buffer << " 易道云编程";

    return 0;
}

int old_test()
{
    //printf("%s 向你问好!\n", "ServerLinux");
    /*
     * procLog：日志服务进程对象。
     * procClients：客户端服务进程对象。
     *
     * 为什么是两个对象：
     * - 每个服务进程都有自己的入口函数和 pid。
     * - 分开保存可以独立启动、独立监控。
     */
     //CProcess::SwitchDeamon();

    CProcess procLog, procClients;

    printf("%s(%d) <%s> pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());

    procLog.SetEntryFunc(CreateLogServer, &procLog);
    int nRet = procLog.CreateSubProc();
    if (nRet != 0)
    {
        printf("%s(%d) <%s> pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
        return -1;
    }

    LogTest();
    CThread thread(LogTest);
    thread.Start();

    printf("%s(%d) <%s> pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
    procClients.SetEntryFunc(CreateClientServer, &procClients);
    nRet = procClients.CreateSubProc();
    if (nRet != 0)
    {
        printf("%s(%d) <%s> pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
        return -2;
    }

    printf("%s(%d) <%s> pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
    usleep(100 * 1000);

    int nFd = open("./test.txt", O_RDWR | O_CREAT | O_APPEND, 0666);
    printf("%s(%d) <%s> fd = %d\n", __FILE__, __LINE__, __FUNCTION__, nFd);
    if (nFd == -1)
    {
        return -3;
    }

    nRet = procClients.nSendFD(nFd);
    printf("%s(%d) <%s> nRet = %d\n", __FILE__, __LINE__, __FUNCTION__, nRet);
    if (nRet != 0)
    {
        printf("%d msg:%s\n", errno, strerror(errno));
    }

    write(nFd, "edoyun", 6);
    close(nFd);

    CThreadPool pool;
    pool.Start(4);

    nRet = pool.AddTask(LogTest);
    printf("%s(%d) <%s> nRet = %d\n", __FILE__, __LINE__, __FUNCTION__, nRet);
    if (nRet != 0)
    {
        printf("%d msg:%s\n", errno, strerror(errno));
    }

    nRet = pool.AddTask(LogTest);
    printf("%s(%d) <%s> nRet = %d\n", __FILE__, __LINE__, __FUNCTION__, nRet);
    if (nRet != 0)
    {
        printf("%d msg:%s\n", errno, strerror(errno));
    }

    nRet = pool.AddTask(LogTest);
    printf("%s(%d) <%s> nRet = %d\n", __FILE__, __LINE__, __FUNCTION__, nRet);
    if (nRet != 0)
    {
        printf("%d msg:%s\n", errno, strerror(errno));
    }

    nRet = pool.AddTask(LogTest);
    printf("%s(%d) <%s> nRet = %d\n", __FILE__, __LINE__, __FUNCTION__, nRet);
    if (nRet != 0)
    {
        printf("%d msg:%s\n", errno, strerror(errno));
    }

    (void)getchar();
    pool.Close();

    procLog.nSendFD(-1);
    (void)getchar();

    return 0;
}

int server_test()
{
	int nRet = 0;
	CProcess procLog;
	CEdoyunPlayerServer business(2);
	CServer server;

	procLog.SetEntryFunc(CreateLogServer, &procLog);
	nRet = procLog.CreateSubProc();
	RET_ERR(nRet, -1);

	nRet = server.Init(&business);
	RET_ERR(nRet, -2);

	nRet = server.Run();
	RET_ERR(nRet, -3);

	return 0;
}

int http_test()
{
    Buffer str = "GET /favicon.ico HTTP/1.1\r\n"
        "Host: 0.0.0.0=5000\r\n"
        "User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9) Gecko/2008061015 Firefox/3.0\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*; q = 0.8\r\n"
        "Accept-Language: en-us,en;q=0.5\r\n"
        "Accept-Encoding: gzip,deflate\r\n"
        "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
        "Keep-Alive: 300\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    CHttpParser parser;
    size_t size = parser.Parser(str);
    if (parser.Errno() != 0)
    {
        printf("errno %d\n", parser.Errno());
        return -1;
    }
    if (size != 368)
    {
        printf("size error:%lld %lld\n", size , str.size());
        return -2;
    }
    printf("Method = %d, Url = %s\n", parser.Method(), (char*)parser.Url());

    str = "GET /favicon.ico HTTP/1.1\r\n"
        "Host: 0.0.0.0=5000\r\n"
        "User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9) Gecko/2008061015 Firefox/3.0\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
    size = parser.Parser(str);
    printf("errno:%d size error:%lld\n", parser.Errno(), size);
    if (parser.Errno() != 0x7F)
    {
        return -3;
    }
    if (size != 0)
    {
        return -4;
    }

    int nRet = 0;
    UrlParser url1("https://www.baidu.com/s?ie=utf8&oe=utf8&wd=httplib&tn=98010089_dg&ch=3");
	nRet = url1.Parser();
	if (nRet != 0)
	{
		printf("urlparser1 failed: %d\n", nRet);
		return -5;
	}
	printf("ie = %s except:utf8\n", (char*)url1["ie"]);
	printf("oe = %s except:utf8\n", (char*)url1["oe"]);
	printf("wd = %s except:httplib\n", (char*)url1["wd"]);
	printf("tn = %s except:98010089_dg\n", (char*)url1["tn"]);
	printf("ch = %s except:3\n", (char*)url1["ch"]);

    UrlParser url2("http://127.0.0.1:19811/?time=144000&salt=9527&user=test&sign=1234567890abcdef");
	nRet = url2.Parser();
	if (nRet != 0)
	{
		printf("urlparser2 failed: %d\n", nRet);
		return -6;
	}
	printf("time = %s except:144000\n", (char*)url2["time"]);
	printf("salt = %s except:9527\n", (char*)url2["salt"]);
	printf("user = %s except:test\n", (char*)url2["user"]);
	printf("sign = %s except:1234567890abcdef\n", (char*)url2["sign"]);
	printf("host:%s port:%d\n", (char*)url2.Host(), url2.Port());

    return 0;
}

#include "Sqlite3Client.h"
DECLARE_TABLE_CLASS(user_test, _sqlite3_table_)
DECLARE_FIELD(TYPE_INT, user_id, NOT_NULL | PRIMARY_KEY | AUTOINCREMENT, "INTEGER", "", "", "")
DECLARE_FIELD(TYPE_VARCHAR, user_qq, NOT_NULL, "VARCHAR", "(15)", "", "")
DECLARE_FIELD(TYPE_VARCHAR, user_phone, NOT_NULL | DEFAULT, "VARCHAR", "(12)", "15954068301", "")
DECLARE_FIELD(TYPE_TEXT, user_name, 0, "TEXT", "", "", "")
DECLARE_TABLE_CLASS_END()

//class user_test : public _sqlite3_table_
//{
//public:
//    virtual PTable Copy() const
//    {
//        return PTable(new user_test(*this));
//    }
//
//    user_test()
//    {
//		Name = "user_test";
//
//		{
//			PField field(new _sqlite3_field_(
//				TYPE_INT,
//				"user_id",
//				NOT_NULL | PRIMARY_KEY | AUTOINCREMENT,
//				"INT",
//				"",
//				"",
//				"")
//			);
//
//			FieldDefine.push_back(field);
//			Fields["user_id"] = field;
//		}
//
//		{
//			PField field(new _sqlite3_field_(
//				TYPE_VARCHAR,
//				"user_qq",
//				NOT_NULL | PRIMARY_KEY | AUTOINCREMENT,
//				"VARCHAR",
//				"(15)",
//				"",
//				"")
//			);
//
//			FieldDefine.push_back(field);
//			Fields["user_id"] = field;
//		}
//    }
//};

int sqlite3_test()
{
    user_test test , value;
    
    printf("Create: %s \n", (char*)test.Create());
    printf("Delete: %s \n", (char*)test.Delete(test));

    value.Fields["user_qq"]->LoadFromStr("1285173093");
    value.Fields["user_qq"]->Condition = SQL_INSERT;
    printf("Insert: %s \n", (char*)test.Insert(value));

    value.Fields["user_qq"]->LoadFromStr("1234567890");
    value.Fields["user_qq"]->Condition = SQL_MODIFY;
	printf("Modify: %s \n", (char*)test.Modify(value));

	printf("Query: %s \n", (char*)test.Query());

	printf("Drop: %s \n", (char*)test.Drop());
    getchar();

	CDatabaseClient* pClient = new CSqlite3Client();
	KeyValue args;
	args["host"] = "test.db";
	int nRet = pClient->Connect(args);
    printf("%s(%d) <%s> Connect nRet = %d\n", __FILE__, __LINE__, __FUNCTION__, nRet);

    nRet = pClient->Exec(test.Create());
    printf("%s(%d) <%s> Create nRet = %d\n", __FILE__, __LINE__, __FUNCTION__, nRet);

    nRet = pClient->Exec(test.Delete(value));
    printf("%s(%d) <%s> Delete nRet = %d\n", __FILE__, __LINE__, __FUNCTION__, nRet);

	value.Fields["user_qq"]->LoadFromStr("1285173093");
	value.Fields["user_qq"]->Condition = SQL_INSERT;
    nRet = pClient->Exec(test.Insert(value));
    printf("%s(%d) <%s> Insert nRet = %d\n", __FILE__, __LINE__, __FUNCTION__, nRet);

	value.Fields["user_qq"]->LoadFromStr("1234567890");
	value.Fields["user_qq"]->Condition = SQL_MODIFY;
    nRet = pClient->Exec(test.Modify(value));
    printf("%s(%d) <%s> Modify nRet = %d\n", __FILE__, __LINE__, __FUNCTION__, nRet);

    Result res;
    nRet = pClient->Exec(test.Query(), res, test);
    printf("%s(%d) <%s> Query nRet = %d\n", __FILE__, __LINE__, __FUNCTION__, nRet);

    nRet = pClient->Exec(test.Drop());
    printf("%s(%d) <%s> Drop nRet = %d\n", __FILE__, __LINE__, __FUNCTION__, nRet);

    nRet = pClient->Close();
    printf("%s(%d) <%s> Close nRet = %d\n", __FILE__, __LINE__, __FUNCTION__, nRet);
    getchar();

    return 0;
}

int main()
{
    /*int nRet = http_test();*/

    int nRet = sqlite3_test();

    printf("main: nRet = %d\n", nRet);

    return nRet;
}
