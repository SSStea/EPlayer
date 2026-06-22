#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <functional>
#include <memory.h>
#include <sys/socket.h>

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

/*
 * 所有“可执行入口”的抽象基类。
 *
 * 为什么需要这个类：
 * - CProcess 需要保存“某个入口函数”，但不同入口函数的类型可能不一样：
 *   有的可能是普通函数，有的可能是成员函数、lambda 或函数对象。
 * - C++ 里不同函数签名对应不同类型，不能直接用一个普通成员变量保存所有情况。
 * - 所以这里用一个基类 CFuncBase 做类型擦除，只暴露统一的 operator() 接口。
 */
class CFuncBase
{
public:
    CFuncBase() {}
    virtual ~CFuncBase() {}

    /*
     * 执行真正的入口函数。
     *
     * 返回值：
     * - 0：通常表示执行成功。
     * - 非 0：通常表示执行失败，具体含义由实际入口函数决定。
     *
     * 为什么 operator() 是纯虚函数：
     * - CFuncBase 只规定“能被调用”这个能力。
     * - 真正怎么调用、调用哪个函数、带哪些参数，由派生类 CFunction 实现。
     */
    virtual int operator()() = 0;

private:

};

/*
 * 具体的函数包装器。
 *
 * 模板参数说明：
 * - _FUNCTION_：入口函数的类型。
 *   例如 CreateLogServer 的类型是 int (*)(CProcess*)。
 * - _ARGS_...：入口函数参数的类型列表。
 *   例如传入 &procLog 时，参数类型列表里就有一个 CProcess*。
 *
 * 为什么用模板：
 * - 不同入口函数的参数数量和参数类型可能不同。
 * - 模板可以在编译期根据实际传入的函数和参数生成对应的包装类。
 *
 * 为什么最后统一变成 int()：
 * - CProcess 创建子进程时只需要“执行入口函数并拿到 int 返回值”。
 * - 具体入口函数原来需要哪些参数，设计上应提前绑定到 m_binder 里。
 */
template<typename _FUNCTION_, typename... _ARGS_>
class CFunction : public CFuncBase
{
public:
    /*
     * 构造函数：用于保存入口函数和入口参数。
     *
     * 参数说明：
     * - func：真正要执行的入口函数。
     *   例如 CreateLogServer 或 CreateClientServer。
     * - args...：入口函数需要的所有参数。
     *   例如 &procLog，表示把当前 CProcess 对象的地址传给日志服务入口。
     *
     * 为什么使用 std::bind：
     * - func 本来可能需要参数，比如 int CreateLogServer(CProcess* proc)。
     * - CProcess 在 CreateSubProc() 里希望统一调用 (*m_func)()，也就是无参调用。
     * - std::bind(func, args...) 会把函数和参数预先绑定起来，生成一个无参可调用对象。
     *
     * 注意：
     * - 当前函数体是空的，说明这里还没有真正把 func 和 args... 保存到 m_binder。
     * - 如果后续只补实现，通常会在构造函数初始化列表里完成绑定。
     */
    CFunction(_FUNCTION_ func, _ARGS_... args)
        : m_binder(std::forward<_FUNCTION_>(func), std::forward<_ARGS_>(args)...)
    {

    }

    virtual ~CFunction(){}

    /*
     * 执行 m_binder 中保存的入口函数。
     *
     * 返回值：
     * - 直接返回入口函数的执行结果。
     *
     * 为什么这里不再传参数：
     * - 设计目标是让参数在构造函数里通过 std::bind 固定到 m_binder 里。
     * - 这样 operator() 只负责无参调用，不需要关心原入口函数需要几个参数。
     */
    virtual int operator()()
    {
        return m_binder();
    }

    /*
     * 保存“无参、返回 int”的入口调用对象类型。
     *
     * 说明：
     * - _Bindres_helper 是标准库内部辅助类型，用来推导 bind 后对象的类型。
     * - int 表示希望绑定后的调用结果是 int。
     * - _FUNCTION_ 表示被绑定的函数类型。
     * - _ARGS_... 表示被绑定的参数类型列表。
     *
     * 这行代码的意图是让 m_binder 保存 std::bind(func, args...) 的结果，
     * 后面 operator() 就可以通过 m_binder() 统一调用入口函数。
     */
    typename std::_Bindres_helper<int, _FUNCTION_, _ARGS_...>::type m_binder;
};

/*
 * 进程包装类。
 *
 * 一个 CProcess 对象对应一个准备启动的子进程。
 * 它负责：
 * - 保存子进程入口函数。
 * - 调用 fork() 创建子进程。
 * - 在父进程中记录子进程 pid。
 */
class CProcess
{
public:
    CProcess()
    {
        m_func = NULL;
        memset(pipes, 0, sizeof(pipes));
    }

    ~CProcess()
    {
        if(m_func != NULL)
        {
            delete m_func;
            m_func = NULL;
        }
    }

    /*
     * 设置子进程入口函数。
     *
     * 模板参数说明：
     * - _FUNCTION_：入口函数类型，由 func 自动推导。
     * - _ARGS_...：入口函数参数类型列表，由 args... 自动推导。
     *
     * 参数说明：
     * - func：子进程启动后要执行的函数。
     * - args...：传给 func 的参数列表，可以是 0 个、1 个或多个。
     *
     * 返回值：
     * - 0：入口函数保存成功。
     *
     * 为什么不直接在这里执行 func：
     * - SetEntryFunc() 只是配置入口。
     * - 真正执行入口函数要等 CreateSubProc() 成功 fork 出子进程之后，
     *   这样入口逻辑才会运行在子进程里，而不是主进程里。
     */
    template<typename _FUNCTION_, typename... _ARGS_>
    int SetEntryFunc(_FUNCTION_ func, _ARGS_... args)
    {
        m_func = new CFunction<_FUNCTION_, _ARGS_...>(func, args...);
        return 0;
    }

    int CreateSubProc()
    {
        if (m_func == NULL)
        {
            return -1;
        }

        int nRet = socketpair(AF_LOCAL, SOCK_STREAM, 0, pipes);//本地套接字
        if (nRet == -1)
        {
            return -2;
        }

        pid_t pid = fork();
        if (pid == -1)
        {
            return -3;
        }
        else if (pid == 0)
        {
            /* 子进程分支*/
            close(pipes[1]);//关闭管道的写，这样子进程就有读管道
            pipes[1] = 0;
            return (*m_func)();
        }

        close(pipes[0]);//关闭管道的读，这样主进程就有写管道
        pipes[0] = 0;
        /* 父进程分支*/
        m_pid = pid;
        return 0;
    }

    int nSendFD(int nFd)//主进程
    {
        iovec iov[2];
        iov[0].iov_base = (char*)"edoyun";
        iov[0].iov_len = 7;
		iov[1].iov_base = (char*)"jeuding";
		iov[1].iov_len = 8;

        cmsghdr* cmsg = new cmsghdr;
        bzero(cmsg, sizeof(cmsghdr));
        if (cmsg == NULL)
        {
            return -1;
        }
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;//套接字级别
        cmsg->cmsg_type = SCM_RIGHTS;//权限
        *(int*)CMSG_DATA(cmsg) = nFd;

		struct msghdr msg;
        msg.msg_iov = iov;
        msg.msg_iovlen = 2;
        msg.msg_control = cmsg;
        msg.msg_controllen = cmsg->cmsg_len;

		ssize_t ret = sendmsg(pipes[1], &msg, 0);
		delete cmsg;
        if (ret == -1)
        {
            return -2;
        }

        return 0;
    }

    int nRecvFd(int& nFd)
    {
        iovec iov[2];
        char buf[][10] = { "", "" };
        iov[0].iov_base = buf[0];
        iov[0].iov_len = sizeof(buf[0]);
		iov[1].iov_base = buf[1];
		iov[1].iov_len = sizeof(buf[1]);

        cmsghdr* cmsg = new cmsghdr;
		bzero(cmsg, sizeof(cmsghdr));
		if (cmsg == NULL)
		{
			return -1;
		}
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
		cmsg->cmsg_level = SOL_SOCKET;//套接字级别
		cmsg->cmsg_type = SCM_RIGHTS;//权限

		msghdr msg;
        msg.msg_iov = iov;
        msg.msg_iovlen = 2;
        msg.msg_control = cmsg;
        msg.msg_controllen = cmsg->cmsg_len;

        ssize_t ret = recvmsg(pipes[0], &msg, 0);
        if (ret == -1)
        {
            delete cmsg;
            return -2;
        }
        nFd = *(int*)CMSG_DATA(cmsg);
        delete cmsg;
        return 0;
    }

private:
    /*
     * 子进程入口函数包装对象。
     *
     * 使用基类指针的原因：
     * - CFunction 是模板类，不同入口函数会生成不同 CFunction 类型。
     * - CProcess 只关心“能调用 operator()”，所以保存 CFuncBase* 即可。
     */
    CFuncBase*  m_func;
    pid_t       m_pid;
    int         pipes[2];//管道，主进程用来传递文件描述符给子进程

};

int CreateLogServer(CProcess* proc)
{
    return 0;
}

int CreateClientServer(CProcess* proc)
{
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
    CProcess procLog, procClients;

    procLog.SetEntryFunc(CreateLogServer, &procLog);
    int nRet = procLog.CreateSubProc();
    if (nRet != 0)
    {
        return -1;
    }
   
    procClients.SetEntryFunc(CreateClientServer, &procClients);
    nRet = procClients.CreateSubProc();
	if (nRet != 0)
	{
		return -2;
	}

    return 0;
}
