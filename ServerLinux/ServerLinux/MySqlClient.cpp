#include "MySqlClient.h"

CMySqlClient::CMySqlClient()
{
	bzero(&m_db, sizeof(MYSQL));
	m_bIsInit = false;
}

int CMySqlClient::Connect(const KeyValue& args)
{
	if (m_bIsInit)
	{
		return -1;
	}

	MYSQL* ret = mysql_init(&m_db);
	if (ret == NULL)
	{
		return -2;
	}

	ret = mysql_real_connect(
		&m_db,
		args.at("host"),
		args.at("user"),
		args.at("password"),
		args.at("db"),
		atoi(args.at("port")),
		NULL,
		0
	);
	if (ret == NULL && mysql_errno(&m_db) != 0)
	{
		TRACEE("errno:%d msg = %s", mysql_errno(&m_db), mysql_error(&m_db));
		mysql_close(&m_db);
		bzero(&m_db, sizeof(MYSQL));
		return -3;
	}

	m_bIsInit = true;

	return 0;
}

int CMySqlClient::Exec(const Buffer& sql)
{
	if (!m_bIsInit)
	{
		return -1;
	}

	int nRet = mysql_real_query(&m_db, sql, sql.size());
	if (nRet != 0)
	{
		printf("%s %s\n", __FUNCTION__, mysql_errno(&m_db));
		TRACEE("errno:%d msg = %s", mysql_errno(&m_db), mysql_error(&m_db));
		return -2;
	}

	return 0;
}

int CMySqlClient::Exec(const Buffer& sql, Result& result, const _Table_& table)
{
	if (!m_bIsInit)
	{
		return -1;
	}

	int nRet = mysql_real_query(&m_db, sql, sql.size());
	if (nRet != 0)
	{
		printf("%s %s\n", __FUNCTION__, mysql_errno(&m_db));
		TRACEE("errno:%d msg = %s", mysql_errno(&m_db), mysql_error(&m_db));
		return -2;
	}

	MYSQL_RES* res = mysql_store_result(&m_db);
	MYSQL_ROW row;
	unsigned num_fields = mysql_num_fields(res);
	while ((row = mysql_fetch_row(res)) != NULL)
	{
		PTable pt = table.Copy();
		for (unsigned i = 0; i < num_fields; i++)
		{
			if (row[i] != NULL)
			{
				pt->FieldDefine[i]->LoadFromStr(row[i]);
			}
		}
		result.push_back(pt);
	}

	return 0;
}

int CMySqlClient::StartTransaction()
{
	if (!m_bIsInit)
	{
		return -1;
	}

	int nRet = mysql_real_query(&m_db, "BEGIN", 6);
	if (nRet != 0)
	{
		printf("%s %s\n", __FUNCTION__, mysql_errno(&m_db));
		TRACEE("errno:%d msg = %s", mysql_errno(&m_db), mysql_error(&m_db));
		return -2;
	}

	return 0;
}

int CMySqlClient::CommitTransaction()
{
	if (!m_bIsInit)
	{
		return -1;
	}

	int nRet = mysql_real_query(&m_db, "COMMIT", 7);
	if (nRet != 0)
	{
		printf("%s %s\n", __FUNCTION__, mysql_errno(&m_db));
		TRACEE("errno:%d msg = %s", mysql_errno(&m_db), mysql_error(&m_db));
		return -2;
	}

	return 0;
}

int CMySqlClient::RollbackTransaction()
{
	if (!m_bIsInit)
	{
		return -1;
	}

	int nRet = mysql_real_query(&m_db, "ROLLBACK", 9);
	if (nRet != 0)
	{
		printf("%s %s\n", __FUNCTION__, mysql_errno(&m_db));
		TRACEE("errno:%d msg = %s", mysql_errno(&m_db), mysql_error(&m_db));
		return -2;
	}

	return 0;
}

int CMySqlClient::Close()
{
	if (m_bIsInit)
	{
		m_bIsInit = false;
		mysql_close(&m_db);
		bzero(&m_db, sizeof(MYSQL));
	}

	return 0;
}

bool CMySqlClient::IsConnected()
{
	return m_bIsInit;
}

_mysql_table_::_mysql_table_(const _mysql_table_& tables)
{
	Database = tables.Database;
	Name = tables.Name;
	for (size_t i = 0; i < tables.FieldDefine.size(); i++)
	{
		// 1. 从 shared_ptr 里取出原来的字段指针
		_mysql_field_* pOldField = (_mysql_field_*)tables.FieldDefine[i].get();
		// 2. 用拷贝构造复制一份新的字段对象
		_mysql_field_* pNewField = new _mysql_field_(*pOldField);
		// 3. 交给 shared_ptr 管理
		PField field(pNewField);

		FieldDefine.push_back(field);
		Fields[field->Name] = field;
	}
}

Buffer _mysql_table_::Create()
{
	//CREATE TABLE IF NOT EXISTS 表全名 (列定义,..., PRIMARY KEY 主键列名, UNIQUE INDEX `列名_UNIQUE` (列名 ASC) VISIBLE);
	Buffer sql = "CREATE TABLE IF NOT EXISTS " + (Buffer)*this + " (\r\n";

	for (unsigned i = 0; i < FieldDefine.size(); i++)
	{
		if (i > 0)
		{
			sql += ",\r\n";
		}
		sql += FieldDefine[i]->Create() + " ";

		if (FieldDefine[i]->Attr & PRIMARY_KEY)
		{
			sql += ",\r\nPRIMARY KEY (`" + FieldDefine[i]->Name + "`)";
		}

		if (FieldDefine[i]->Attr & UNIQUE)
		{
			sql += ",\r\nUNIQUE INDEX `" + FieldDefine[i]->Name + "_UNIQUE` ("
				+ (Buffer)*FieldDefine[i] + " ASC) VISIBLE";
		}
	}
	sql += " );";

	return sql;
}

Buffer _mysql_table_::Drop()
{
	return "DROP TABLE" + (Buffer)*this;
}

Buffer _mysql_table_::Insert(const _Table_& values)
{
	//INSERT INTO 表名 (列名,...)VALUES(值,...);
	Buffer sql = "INSERT INTO " + (Buffer)*this + " (";
	bool bIsFirst = true;
	for (size_t i = 0; i < values.FieldDefine.size(); i++)
	{
		if (values.FieldDefine[i]->Condition & SQL_INSERT)
		{
			if (!bIsFirst)
			{
				sql += ',';
			}
			else
			{
				bIsFirst = false;
			}
			sql += (Buffer)*values.FieldDefine[i];
		}
	}

	sql += ") VALUES (";
	bIsFirst = true;
	for (size_t i = 0; i < values.FieldDefine.size(); i++)
	{
		if (values.FieldDefine[i]->Condition & SQL_INSERT)
		{
			if (!bIsFirst)
			{
				sql += ',';
			}
			else
			{
				bIsFirst = false;
			}
			sql += values.FieldDefine[i]->toSqlString();
		}
	}
	sql += " );";

	printf("sql = %s\n", (char*)sql);

	return sql;

	return Buffer();
}

Buffer _mysql_table_::Delete(const _Table_& values)
{
	//DELET FROM 表全名 WHERE 条件
	Buffer sql = "DELETE FROM " + (Buffer)*this + " ";
	Buffer where = "";
	bool bIsFirst = true;
	for (size_t i = 0; i < FieldDefine.size(); i++)
	{
		if (FieldDefine[i]->Condition & SQL_CONDITION)
		{
			if (!bIsFirst)
			{
				sql += " AND ";
			}
			else
			{
				bIsFirst = false;
			}
			where += (Buffer)*FieldDefine[i] + "=" + FieldDefine[i]->toSqlString();
		}
	}
	if (where.size() > 0)
	{
		sql = sql + "WHERE " + where;
	}
	sql += ";";

	printf("sql = %s\n", (char*)sql);

	return sql;
}

Buffer _mysql_table_::Modify(const _Table_& values)
{
	//UPDATE 表全名 SET 列1=值1, ..., 列n=值n [WHERE 条件]
	Buffer sql = "UPDATE " + (Buffer)*this + " SET ";
	bool bIsFirst = true;
	for (size_t i = 0; i < values.FieldDefine.size(); i++)
	{
		if (values.FieldDefine[i]->Condition & SQL_MODIFY)
		{
			if (!bIsFirst)
			{
				sql += ',';
			}
			else
			{
				bIsFirst = false;
			}
			sql += (Buffer)*values.FieldDefine[i] + "=" + values.FieldDefine[i]->toSqlString();
		}
	}

	Buffer where = "";
	bIsFirst = true;
	for (size_t i = 0; i < values.FieldDefine.size(); i++)
	{
		if (values.FieldDefine[i]->Condition & SQL_CONDITION)
		{
			if (!bIsFirst)
			{
				sql += " AND ";
			}
			else
			{
				bIsFirst = false;
			}
			where += (Buffer)*values.FieldDefine[i] + "=" + values.FieldDefine[i]->toSqlString();
		}
	}
	if (where.size() > 0)
	{
		sql = sql + "WHERE " + where;
	}
	sql += " ;";

	printf("sql = %s\n", (char*)sql);

	return sql;
}

Buffer _mysql_table_::Query(const Buffer& condition)
{
	//SELECT 列名1,列名2,...,列名n FROM 表全名;
	Buffer sql = "SELECT ";

	for (size_t i = 0; i < FieldDefine.size(); i++)
	{
		if (i > 0)
		{
			sql += ',';
		}
		sql = sql + '`' + FieldDefine[i]->Name + "` ";
	}

	sql = sql + " FROM " + (Buffer)*this;
	if (condition.size() > 0)
	{
		sql += " WHERE " + condition;
	}
	sql += ";";

	printf("sql = %s\n", (char*)sql);

	return sql;
}

PTable _mysql_table_::Copy() const
{
	return PTable(new _mysql_table_(*this));
}

void _mysql_table_::ClearFieldUsed()
{
	for (size_t i = 0; i < FieldDefine.size(); i++)
	{
		FieldDefine[i]->Condition = 0;
	}
}

_mysql_table_::operator const Buffer() const
{
	Buffer Head;
	if (Database.size())
	{
		Head = "`" + Database + "`.";
	}

	return Head + '`' + Name + '`';
}

_mysql_field_::_mysql_field_() : _Field_()
{
	nType = TYPE_NULL;
}

_mysql_field_::_mysql_field_(SqlType ntype, const Buffer& name, unsigned attr, const Buffer& type, const Buffer& size, const Buffer& default_, const Buffer& check)
{
	nType = ntype;
	Name = name;
	Attr = attr;
	Type = type;
	Size = size;
	Default = default_;
	Check = check;
}


Buffer _mysql_field_::Create()
{
	Buffer sql = '`' + Name + "` " + Type + Size + " ";

	if (Attr & NOT_NULL)
	{
		sql += "NOT NULL";
	}
	else
	{
		sql += "NULL";
	}

	//BLOB TEXT GEOMETRY JSON不能有默认值
	if (Type != "BLOB" && Type != "TEXT" && Type != "GEOMETRY" && Type != "JSON")
	{
		if (Attr & DEFAULT && Default.size() > 0)
		{
			sql += " DEFAULT  \"" + Default + "\" ";
		}
	}

	//UNIQUE 和PRIMARY KEY 在外面处理
	//CHECK mysql不支持
	if (Attr & AUTOINCREMENT)
	{
		sql += " AUTO_INCREMENT ";
	}

	return sql;
}

void _mysql_field_::LoadFromStr(const Buffer& str)
{
	switch (nType)
	{
	case TYPE_NULL:
		break;
	case TYPE_BOOL:
	case TYPE_INT:
	case TYPE_DATETIME:
		Value.Integer = atoi(str);
		break;
	case TYPE_REAL:
		Value.Decimal = atof(str);
		break;
	case TYPE_VARCHAR:
	case TYPE_TEXT:
		Value.String = str;
		break;
	case TYPE_BLOB:
		Value.String = Str2Hex(str);
		break;
	default:
		char buf[128] = "";
		snprintf(buf, sizeof(buf), "send client fd %d failed!", nType);
		printf("%s\n", buf);
		break;
	}
}

Buffer _mysql_field_::toEqualExp() const
{
	Buffer sql = (Buffer)*this + " = ";
	std::stringstream ss;

	switch (nType)
	{
	case TYPE_NULL:
		sql += "NULL ";
		break;
	case TYPE_BOOL:
	case TYPE_INT:
	case TYPE_DATETIME:
		ss << Value.Integer;
		sql += ss.str() + " ";
		break;
	case TYPE_REAL:
		ss << Value.Decimal;
		sql += ss.str() + " ";
		break;
	case TYPE_VARCHAR:
	case TYPE_TEXT:
	case TYPE_BLOB:
		sql += '"' + Value.String + "\" ";
		break;
	default:
		char buf[128] = "";
		snprintf(buf, sizeof(buf), "send client fd %d failed!", nType);
		printf("%s\n", buf);
		break;
	}

	return sql;
}

Buffer _mysql_field_::toSqlString() const
{
	Buffer sql = "";
	std::stringstream ss;

	switch (nType)
	{
	case TYPE_NULL:
		sql += "NULL ";
		break;
	case TYPE_BOOL:
	case TYPE_INT:
	case TYPE_DATETIME:
		ss << Value.Integer;
		sql += ss.str() + " ";
		break;
	case TYPE_REAL:
		ss << Value.Decimal;
		sql += ss.str() + " ";
		break;
	case TYPE_VARCHAR:
	case TYPE_TEXT:
	case TYPE_BLOB:
		sql += '"' + Value.String + "\" ";
		break;
	default:
		char buf[128] = "";
		snprintf(buf, sizeof(buf), "send client fd %d failed!", nType);
		printf("%s\n", buf);
		break;
	}

	return sql;
}

_mysql_field_::operator const Buffer() const
{
	return '`' + Name + '`';
}

Buffer _mysql_field_::Str2Hex(const Buffer& data) const
{
	const char* hex = "0123456789ABCDEF";
	std::stringstream ss;
	for (auto& ch : data)
	{
		ss << hex[(unsigned char)ch >> 4] << hex[(unsigned char)ch & 0xF];
	}

	return ss.str();
}