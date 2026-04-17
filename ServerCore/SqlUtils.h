#pragma once

#include <string>
#include <bcrypt/BCrypt.hpp>

class SqlUtils
{

private:
    // 쿼리문에 변수 대입
    // 다양한 파라미터 타입에 대응하여 바인딩시켜주는 함수
    template<typename T>
    static void bindParam(sql::PreparedStatement* stmt, int index, T&& value)
    {
        if constexpr (std::is_same_v<std::decay_t<T>, uint64>)
        {
            stmt->setUInt64(index, value);
        }
        else if constexpr (std::is_same_v<std::decay_t<T>, int64>)
        {
            stmt->setInt64(index, value);
        }
        else if constexpr (std::is_same_v<std::decay_t<T>, uint32>)
        {
            stmt->setUInt(index, value);
        }
        else if constexpr (std::is_same_v<std::decay_t<T>, int32>)
        {
            stmt->setInt(index, value);
        }
        else if constexpr (std::is_same_v<std::decay_t<T>, std::string> ||
            std::is_same_v<std::decay_t<T>, sql::SQLString>)
        {
            stmt->setString(index, value);
        }
        else if constexpr (std::is_same_v<std::decay_t<T>, double>)
        {
            stmt->setDouble(index, value);
        }
        else if constexpr (std::is_same_v<std::decay_t<T>, bool>)
        {
            stmt->setBoolean(index, value);
        }
    }

    // Args 요소 하나씩 찾아가서 BindParam 호출
    template<typename T, typename... Args>
    static void bindParams(sql::PreparedStatement* stmt, int index, T&& value, Args&&... args)
    {
        bindParam(stmt, index, std::forward<T>(value));
        if constexpr (sizeof...(args) > 0)
        {
            bindParams(stmt, index + 1, std::forward<Args>(args)...);
        }
    }

public:
    /* 쿼리 하나 만드는데 필요한 정보(Parameters)
     * Schema(DB 이름)
     * SqlConnectionPtr
     * DML Content
     *
     * 반환 결과(Response)
     * ResultSet
     */
    template<typename... Args>
    static std::shared_ptr<sql::ResultSet> executeQuery(
        std::shared_ptr<sql::Connection> sql_conn,
        const sql::SQLString& schema,
        const sql::SQLString& query,
        Args&&... args)
    {
        sql_conn->setSchema(schema);
        std::unique_ptr<sql::PreparedStatement> pstmt(sql_conn->prepareStatement(query));
        if constexpr (sizeof...(args) > 0)
        {
            bindParams(pstmt.get(), 1, std::forward<Args>(args)...);
        }
        return std::shared_ptr<sql::ResultSet>(pstmt->executeQuery());
    }

    template <typename... Args>
    static int executeUpdate(
        std::shared_ptr<sql::Connection> sql_conn,
        const sql::SQLString& schema,
        const sql::SQLString& query,
        Args&&... args)
    {
        sql_conn->setSchema(schema);
        std::unique_ptr<sql::PreparedStatement> pstmt(sql_conn->prepareStatement(query));

        if constexpr (sizeof...(args) > 0)
        {
            bindParams(pstmt.get(), 1, std::forward<Args>(args)...);
        }

        return pstmt->executeUpdate();
    }

    template <typename... Args>
    static int executeUpdate_GenKeys(
        std::shared_ptr<sql::Connection> sql_conn,
        const sql::SQLString& schema,
        const sql::SQLString& query,
        Args&&... args)
    {
        sql_conn->setSchema(schema);
        std::unique_ptr<sql::PreparedStatement> pstmt(sql_conn->prepareStatement(query));
            
        if constexpr (sizeof...(args) > 0)
        {
            bindParams(pstmt.get(), 1, std::forward<Args>(args)...);
        }

        pstmt->executeUpdate();

        // 가장 최근에 생성된 키 가져오기
        std::unique_ptr<sql::Statement> stmt(sql_conn->createStatement());
        std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery("SELECT LAST_INSERT_ID()"));

        int id = -1;
        if (rs->next())
            id = rs->getInt(1);

        return id;
    }

    // 비밀번호 해시 생성 (회원가입 시)
    static std::string HashPassword(const std::string& password)
    {
        return BCrypt::generateHash(password, 12);  // cost = 12
    }

    // 비밀번호 검증 (로그인 시)
    static bool VerifyPassword(const std::string& password, const std::string& hash) 
    {
        return BCrypt::validatePassword(password, hash);
    }
};

