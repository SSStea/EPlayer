#include "MySqlClient.h"

CMySqlClient::CMySqlClient()
{
	bzero(&m_db, sizeof(MYSQL));
}

int CMySqlClient::Connect(const KeyValue& args)
{
	return 0;
}

int CMySqlClient::Exec(const Buffer& sql)
{
	return 0;
}

int CMySqlClient::Exec(const Buffer& sql, Result& result, const _Table_& table)
{
	return 0;
}

int CMySqlClient::StartTransaction()
{
	return 0;
}

int CMySqlClient::CommitTransaction()
{
	return 0;
}

int CMySqlClient::RollbackTransaction()
{
	return 0;
}

int CMySqlClient::Close()
{
	return 0;
}

bool CMySqlClient::IsConnected()
{
	return false;
}

int CMySqlClient::ExecCallback(void* arg, int count, char** values, char** names)
{
	return 0;
}

int CMySqlClient::ExecCallback(Result& result, const _Table_& table, int count, char** values, char** names)
{
	return 0;
}

_mysql_table_::_mysql_table_(const _mysql_table_& tables)
{

}

Buffer _mysql_table_::Create()
{
	return Buffer();
}

Buffer _mysql_table_::Drop()
{
	return Buffer();
}

Buffer _mysql_table_::Insert(const _Table_& values)
{
	return Buffer();
}

Buffer _mysql_table_::Delete(const _Table_& values)
{
	return Buffer();
}

Buffer _mysql_table_::Modify(const _Table_& values)
{
	return Buffer();
}

Buffer _mysql_table_::Query()
{
	return Buffer();
}

PTable _mysql_table_::Copy() const
{
	return PTable();
}

void _mysql_table_::ClearFieldUsed()
{

}

_mysql_table_::operator const Buffer() const
{
	return Buffer();
}

_mysql_field_::_mysql_field_()
{

}

_mysql_field_::_mysql_field_(SqlType ntype, const Buffer& name, unsigned attr, const Buffer& type, const Buffer& size, const Buffer& default_, const Buffer& check)
{

}


Buffer _mysql_field_::Create()
{
	return Buffer();
}

void _mysql_field_::LoadFromStr(const Buffer& str)
{

}

Buffer _mysql_field_::toEqualExp() const
{
	return Buffer();
}

Buffer _mysql_field_::toSqlString() const
{
	return Buffer();
}

_mysql_field_::operator const Buffer() const
{
	return Buffer();
}

Buffer _mysql_field_::Str2Hex(const Buffer& data) const
{
	return Buffer();
}