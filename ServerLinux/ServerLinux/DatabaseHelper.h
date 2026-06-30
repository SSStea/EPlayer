#pragma once
#include "Public.h"
#include <map>
#include <list>
#include <memory>
#include <vector>

class _Table_;
using PTable = std::shared_ptr<_Table_>;

using KeyValue = std::map<Buffer, Buffer>;
using Result = std::list<PTable>;

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

enum 
{
	SQL_INSERT = 1, //插入的列
	SQL_MODIFY = 2, //修改的列
	SQL_CONDITION = 4, //查询条件
};

enum {
	NOT_NULL = 1,
	DEFAULT = 2,
	UNIQUE = 4,
	PRIMARY_KEY =8,
	CHECK = 16,
	AUTOINCREMENT = 32
};

class _Field_
{
public:
	_Field_() {}
	virtual ~_Field_() {}

	_Field_(const _Field_& field);
	virtual _Field_& operator=(const _Field_& field);

public:
	virtual Buffer Create() = 0;
	virtual void LoadFromStr(const Buffer& str) = 0;
	virtual Buffer toEqualExp() const = 0;//where语句使用
	virtual Buffer toSqlString() const = 0;
	virtual operator const Buffer() const = 0;//列的全名

public:
	unsigned Condition;//操作条件
	Buffer Name;
	Buffer Type;
	Buffer Size;
	unsigned Attr;
	Buffer Default;
	Buffer Check;
};

using PField = std::shared_ptr<_Field_>;
using FieldArray = std::vector<PField>;
using FieldMap = std::map<Buffer, PField>;

class _Table_
{
public:
	_Table_(){}
	virtual ~_Table_(){}

public:
	virtual Buffer Create() = 0;//返回对创建表操作的sql语句，真正的执行都是用Exec()
	virtual Buffer Drop() = 0;//返回对删除表操作的sql语句

	virtual Buffer Insert(const _Table_& values) = 0;//增删改查
	virtual Buffer Delete(const _Table_& values) = 0;
	virtual Buffer Modify(const _Table_& values) = 0;
	virtual Buffer Query() = 0;

	virtual PTable Copy() const = 0;//创建一个基于表的对象，
	//方便数据库客户端基类带结果的执行函数使用，针对不同种类的数据库创建不同的对象

	virtual void ClearFieldUsed() = 0;

public:
	virtual operator const Buffer() const = 0;//获取表的全名

	Buffer Database;//表所属的数据库的名称
	Buffer Name;
	FieldArray FieldDefine; //列的定义，存储查询结果
	FieldMap Fields;//列的定义映射表
};

#define DECLARE_TABLE_CLASS(name, dbase) class name : public dbase { \
public: \
virtual PTable Copy() const {return PTable(new name(*this));} \
name():dbase(){Name = #name;

#define DECLARE_FIELD(ntype, name, attr, type, size, default_, check)\
{PField field(new _sqlite3_field_(ntype, #name, attr, type, size, default_, check));\
FieldDefine.push_back(field); Fields[#name] = field;}

#define DECLARE_TABLE_CLASS_END() }};
