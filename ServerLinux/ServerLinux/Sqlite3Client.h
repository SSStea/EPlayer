#pragma once
#include "Public.h"
#include "DatabaseHelper.h"
#include "sqlite3.h"
#include "Logger.h"

class CSqlite3Client
	:public CDatabaseClient
{
public:
	CSqlite3Client();
	virtual ~CSqlite3Client();

	CSqlite3Client(const CSqlite3Client&) = delete;
	CSqlite3Client& operator=(const CSqlite3Client&) = delete;

public:
	virtual int Connect(const KeyValue& args);//连接
	virtual int Exec(const Buffer& sql);//执行
	virtual int Exec(const Buffer& sql, Result& result, const _Table_& table);//带结果的执行
	virtual int StartTransaction(); //开启事务
	virtual int CommitTransaction();//提交事务
	virtual int RollbackTransaction();//回滚事务
	virtual int Close();//关闭
	virtual bool IsConnected();//判断是否连接上

private:
	sqlite3_stmt* m_stmt;
	sqlite3* m_db;

	static int ExecCallback(void* arg, int count, char** values, char** names);
	int ExecCallback(Result& result, const _Table_& table, int count, char** values, char** names);

	class ExecParam
	{
	public:
		ExecParam(CSqlite3Client* obj, Result& result, const _Table_& table)
			: obj(obj), result(result), table(table) { }

		CSqlite3Client* obj;
		Result& result;
		const _Table_& table;
	};
};

class _sqlite3_table_
	:public _Table_
{
public:
	_sqlite3_table_() : _Table_() {}
	virtual ~_sqlite3_table_() {}

	_sqlite3_table_(const _sqlite3_table_& tables);

public:
	virtual Buffer Create();//返回对创建表操作的sql语句，真正的执行都是用Exec()
	virtual Buffer Drop();//返回对删除表操作的sql语句

	virtual Buffer Insert(const _Table_& values);//增删改查
	virtual Buffer Delete(const _Table_& values);
	virtual Buffer Modify(const _Table_& values);
	virtual Buffer Query();

	virtual PTable Copy() const;//创建一个基于表的对象，
	//方便数据库客户端基类带结果的执行函数使用，针对不同种类的数据库创建不同的对象

	virtual void ClearFieldUsed();

public:
	virtual operator const Buffer() const;//获取表的全名
};

class _sqlite3_field_
	:public _Field_
{
public:
	_sqlite3_field_();
	_sqlite3_field_(
		SqlType ntype,
		const Buffer& name,
		unsigned attr,
		const Buffer& type,
		const Buffer& size,
		const Buffer& default_,
		const Buffer& check
	);
	virtual ~_sqlite3_field_() {}

	virtual Buffer Create();
	virtual void LoadFromStr(const Buffer& str);
	virtual Buffer toEqualExp() const;//where语句使用
	virtual Buffer toSqlString() const;
	virtual operator const Buffer() const;//列的全名

private:
	struct {
		bool	Bool;
		int		Integer;
		double	Decimal;
		Buffer	String;
	}Value;
	SqlType nType;

	Buffer Str2Hex(const Buffer& data) const;
};