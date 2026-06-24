#include "Process.h"
#include "Logger.h"

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

int main()
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
        printf("%d msg:%s", errno, strerror(errno));
    }

    write(nFd, "edoyun", 6);
    close(nFd);

    procLog.nSendFD(-1);
    (void)getchar();

    return 0;
}
