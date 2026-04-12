#pragma once

#include <mysql.h>	



class DBManager
{
public:
	DBManager();
	~DBManager();

	void loadmysql(const char mysqlip[], MYSQL* cons);
	void InputQuery(const char* query);

private:
	MYSQL* cons;
	MYSQL_RES* sql_result;
	MYSQL_ROW sql_row;
};

