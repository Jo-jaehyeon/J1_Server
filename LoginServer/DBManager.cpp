#include "pch.h"
#include "DBManager.h"

#define MYSQLUSER "root"				//유저 이름
#define MYSQLPASSWORD "OmegaAlpha"		//비밀번호
#define MYSQLIP "localhost"				//연결할 ip 자기자신은 localhost 아니면 ipconfig로 자신의 ip를 친다.
										//localhost는 다른컴퓨터로 하면 연결이 안된다. 배포할때 주의


DBManager::DBManager()
{
	MYSQL* cons = mysql_init(NULL);
	loadmysql(MYSQLIP, cons);
}

DBManager::~DBManager()
{
	mysql_close(cons);
}

void DBManager::loadmysql(const char mysqlip[], MYSQL* cons)		//MYSQL 서버 불러오기
{
	if (cons == NULL) //cons가 초기화를 못했으면
	{
		fprintf(stderr, "%s\n", mysql_error(cons));					//에러 프린트, printf("%s")와 같음
		Sleep(1000);
		exit(1);
	}
	if (!(mysql_real_connect(cons, mysqlip, MYSQLUSER, MYSQLPASSWORD, NULL, 0, NULL, 0) == NULL)) //mysql서버로 연결 성공하면 0 아니면 다른수 반환
	{
		printf("\n성공");
		mysql_set_character_set(cons, "euckr");						//MySQL 문자를 지정한다. 만약 안하면 한글이 짤림.


	}
	else {
		fprintf(stderr, "연결 오류 : %s\n", mysql_error(cons));
		getchar();
	}

	return;
}

void DBManager::InputQuery(const char* query)
{
	mysql_query(cons, query);
	sql_result = mysql_store_result(cons);
	sql_row = mysql_fetch_row(sql_result);
	printf("%s %s %s", sql_row[0], sql_row[1], sql_row[2]);
}
