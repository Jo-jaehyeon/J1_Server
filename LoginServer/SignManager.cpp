#include "pch.h"
#include "SignManager.h"

#include "DB/ConnectionPool.h"
#include "DB/MySQLConnection.h"
#include "SqlUtils.h"

SignManagerPtr GSignManager = std::make_shared<SignManager>();

SignManager::SignManager()
{
	Query_CheckID = "SELECT account_id, user_password "
		"FROM account "
		"WHERE is_active = 1 AND user_id = ?";

	Query_SignIn = "UPDATE account "
		"SET last_login = NOW() "
		"WHERE user_id = ?";
	Query_Signtoken = "INSERT INTO sessions (account_id, token) "
		"VALUES(?, ?)";

	Query_SignUp = "INSERT INTO account (user_id, user_password, created_at, is_active) "
		"VALUES(?, ?, NOW(), 1)";
}

bool SignManager::TryCheckID(const std::string& id)
{
	auto conn = GConnectionPool->borrow();
	while (conn == nullptr)
	{
		conn = GConnectionPool->borrow();
	}

	auto result_set = SqlUtils::executeQuery(conn->sql_connection, "J1_DB", Query_CheckID, id);

	bool result = false;
	// 결과 처리
	if (result_set)
	{
		result = result_set->next(); // next() 호출해서 커서 이동
		result_set->close();         // 명시적으로 닫기
	}
	
	if (result)
		spdlog::info("can't make this id");
	else
		spdlog::info("can make this id");
	
	GConnectionPool->unborrow(conn);

	return result;
}

std::pair<int, std::string> SignManager::TrySignIn(const std::string& id, const std::string& pw)
{
	auto conn = GConnectionPool->borrow();
	while (conn == nullptr)
	{
		conn = GConnectionPool->borrow();
	}
	// 검증
	auto result_set = SqlUtils::executeQuery(conn->sql_connection, "J1_DB", Query_CheckID, id);

	
	// 입력한 id에 대한 pw hash값 가져오기
	int account_id = -1;
	std::string hashed_pw;
	if (result_set && result_set->next())
	{
		account_id = result_set->getInt("account_id");
		hashed_pw = result_set->getString("user_password");
	}

	// 입력한 pw와 hash값 비교
	bool bSuccess = SqlUtils::VerifyPassword(pw, hashed_pw);

	pair<int, string> result = make_pair(-1, "");
	if (bSuccess)
	{
		spdlog::info("Success to match id&pw");

		// 로그인 토큰 생성 후 insert
		int successInsert = 0;
		do
		{
			try
			{
				string token = SqlUtils::GenerateToken();
				successInsert = SqlUtils::executeUpdate(conn->sql_connection, "J1_DB", Query_Signtoken, account_id, token);
				result.first = account_id;
				result.second = token;
			}
			catch (...)
			{
				// token 중복 - 재시도
				successInsert = -1;
			}
		} while (successInsert < 0);

		//마지막 로그인 정보 갱신
		int successUpdate = SqlUtils::executeUpdate(conn->sql_connection, "J1_DB", Query_SignIn, id);
		if (successUpdate > 0)
			spdlog::info("Success to update id info");	
	}
	else
		spdlog::info("Failed to match id&pw");

	GConnectionPool->unborrow(conn);

	return result;
}

bool SignManager::TrySignUp(const std::string& id, const std::string& pw)
{
	auto conn = GConnectionPool->borrow();
	while (conn == nullptr)
	{
		conn = GConnectionPool->borrow();
	}

	// 회원가입 - 해시 생성
	std::string hash = SqlUtils::HashPassword(pw);

	int account_id = SqlUtils::executeUpdate_GenKeys(conn->sql_connection, "J1_DB", Query_SignUp, id, hash);

	// acount_id가 -1 이면 실패한것
	bool success = account_id != -1;

	// 결과 처리

	if (success)
		spdlog::info("Success to make id");
	else
		spdlog::info("Failed to make id");

	GConnectionPool->unborrow(conn);

	return success;
}
