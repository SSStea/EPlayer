#pragma once
#include "Public.h"
#include "DatabaseHelper.h"
#include "Logger.h"
#include <mysql/mysql.h>

class CMySqlClient
	:public CDatabaseClient
{
public:
	CMySqlClient();
	virtual ~CMySqlClient() {}

	CMySqlClient(const CMySqlClient&) = delete;
	CMySqlClient& operator=(const CMySqlClient&) = delete;

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
	MYSQL m_db;
	bool m_bIsInit;//默认是false，表示没有初始化

	class ExecParam
	{
	public:
		ExecParam(CMySqlClient* obj, Result& result, const _Table_& table)
			: obj(obj), result(result), table(table) {
		}

		CMySqlClient* obj;
		Result& result;
		const _Table_& table;
	};
};

class _mysql_table_
	:public _Table_
{
public:
	_mysql_table_() : _Table_() {}
	virtual ~_mysql_table_() {}

	_mysql_table_(const _mysql_table_& tables);

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

class _mysql_field_
	:public _Field_
{
public:
	_mysql_field_();
	_mysql_field_(
		SqlType ntype,
		const Buffer& name,
		unsigned attr,
		const Buffer& type,
		const Buffer& size,
		const Buffer& default_,
		const Buffer& check
	);
	virtual ~_mysql_field_() {}

	virtual Buffer Create();
	virtual void LoadFromStr(const Buffer& str);
	virtual Buffer toEqualExp() const;//where语句使用
	virtual Buffer toSqlString() const;
	virtual operator const Buffer() const;//列的全名

private:

	Buffer Str2Hex(const Buffer& data) const;
};

#define DECLARE_TABLE_CLASS(name, dbase) class name : public dbase { \
public: \
virtual PTable Copy() const {return PTable(new name(*this));} \
name():dbase(){Name = #name;

#define DECLARE_MYSQL_FIELD(ntype, name, attr, type, size, default_, check)\
{PField field(new _mysql_field_(ntype, #name, attr, type, size, default_, check));\
FieldDefine.push_back(field); Fields[#name] = field;}

#define DECLARE_TABLE_CLASS_END() }};