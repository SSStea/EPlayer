#pragma once
/*
 * 进程包装类。
 *
 * 一个 CProcess 对象对应一个准备启动的子进程。
 * 它负责：
 * - 保存子进程入口函数。
 * - 调用 fork() 创建子进程。
 * - 在父进程中记录子进程 pid。
 */
#include "Function.h"

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
        if (m_func != NULL)
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
            nRet = (*m_func)();
            exit(0);
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
        char buf[2][10] = { "edoyun", "jeuding" };
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
        *(int*)CMSG_DATA(cmsg) = nFd;

        struct msghdr msg;
        memset(&msg, 0, sizeof(msghdr));
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

        struct msghdr msg;
        memset(&msg, 0, sizeof(msghdr));
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

    static int SwitchDeamon()//切换到守护进程
    {
        pid_t ret = fork();
        if (ret == -1)
        {
            return -1;
        }
        if (ret > 0)
        {
            exit(0); //主进程退出
        }

        //子进程工作如下
        ret = setsid();
        if (ret == -1)
        {
            return -2;
        }
        ret = fork();
        if (ret == -1)
        {
            return -3;
        }
        if (ret > 0)
        {
            exit(0); //子进程退出
        }

        //孙进程：进入守护状态
        for (int i = 0; i < 3; i++)
        {
            close(i);
        }
        umask(0);
        signal(SIGCHLD, SIG_IGN);
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
    CFuncBase* m_func;
    pid_t       m_pid;
    int         pipes[2];//管道，主进程用来传递文件描述符给子进程

};