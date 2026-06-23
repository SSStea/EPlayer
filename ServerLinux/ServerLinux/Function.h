#pragma once
/*
 * 所有“可执行入口”的抽象基类。
 *
 * 为什么需要这个类：
 * - CProcess 需要保存“某个入口函数”，但不同入口函数的类型可能不一样：
 *   有的可能是普通函数，有的可能是成员函数、lambda 或函数对象。
 * - C++ 里不同函数签名对应不同类型，不能直接用一个普通成员变量保存所有情况。
 * - 所以这里用一个基类 CFuncBase 做类型擦除，只暴露统一的 operator() 接口。
 */

#include <unistd.h>
#include <sys/types.h>
#include <functional>
#include <memory.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

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

    virtual ~CFunction() {}

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