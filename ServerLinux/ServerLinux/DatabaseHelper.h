#pragma once
#include "Public.h"
#include <map>
#include <list>

class _Table_;

using KeyValue = std::map<Buffer, Buffer>;
using Result = std::list<_Table_>;

class CDatabaseClient
{
public:
	CDatabaseClient() {}
	virtual ~CDatabaseClient() {}

	CDatabaseClient(const CDatabaseClient&) = delete;
	CDatabaseClient& operator=(const CDatabaseClient&) = delete;

public:
	virtual int Connect(const KeyValue& args) = 0;//连接
	virtual int Exec(const Buffer& sql) = 0;//执行
	virtual int Exec(const Buffer& sql, Result& result, const _Table_& table) = 0;//带结果的执行
	virtual int StartTransaction() = 0; //开启事务
	virtual int CommitTransaction() = 0;//提交事务
	virtual int RollbackTransaction() = 0;//回滚事务
	virtual int Close() = 0;//关闭
	virtual bool IsConnected() = 0;//判断是否连接上
};

