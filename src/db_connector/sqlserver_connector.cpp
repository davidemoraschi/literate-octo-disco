#include "db_connector/sqlserver_connector.h"

#include <stdexcept>

#ifdef _WIN32
// windows.h must come first: sqltypes.h relies on the Windows base types and
// macros (e.g. _W64, __int64 typedefs) and fails to parse without them.
#include <windows.h>
#include <sql.h>
#include <sqltypes.h>
#include <sqlext.h>
#endif

namespace sqlbot {

void SqlServerConnector::Connect(const std::string& connection_string) {
    if (connection_string.empty()) {
        throw std::runtime_error("SQL Server connection string is empty.");
    }
    connection_string_ = connection_string;
}

QueryResult SqlServerConnector::Execute(const std::string& sql) {
    if (connection_string_.empty()) {
        throw std::runtime_error("SQL Server is not connected.");
    }
#ifdef _WIN32
    SQLHENV env = nullptr;
    SQLHDBC dbc = nullptr;
    SQLHSTMT stmt = nullptr;

    if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env) != SQL_SUCCESS) {
        throw std::runtime_error("SQLAllocHandle(SQL_HANDLE_ENV) failed");
    }
    SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);

    if (SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc) != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_ENV, env);
        throw std::runtime_error("SQLAllocHandle(SQL_HANDLE_DBC) failed");
    }

    SQLCHAR out_conn[1024]{};
    SQLSMALLINT out_len = 0;
    const SQLRETURN conn_rc = SQLDriverConnectA(
        dbc,
        nullptr,
        reinterpret_cast<SQLCHAR*>(const_cast<char*>(connection_string_.c_str())),
        SQL_NTS,
        out_conn,
        static_cast<SQLSMALLINT>(sizeof(out_conn)),
        &out_len,
        SQL_DRIVER_NOPROMPT);

    if (!SQL_SUCCEEDED(conn_rc)) {
        SQLFreeHandle(SQL_HANDLE_DBC, dbc);
        SQLFreeHandle(SQL_HANDLE_ENV, env);
        throw std::runtime_error("SQLDriverConnectA failed for SQL Server connection string");
    }

    if (SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt) != SQL_SUCCESS) {
        SQLDisconnect(dbc);
        SQLFreeHandle(SQL_HANDLE_DBC, dbc);
        SQLFreeHandle(SQL_HANDLE_ENV, env);
        throw std::runtime_error("SQLAllocHandle(SQL_HANDLE_STMT) failed");
    }

    if (!SQL_SUCCEEDED(SQLExecDirectA(stmt, reinterpret_cast<SQLCHAR*>(const_cast<char*>(sql.c_str())), SQL_NTS))) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        SQLDisconnect(dbc);
        SQLFreeHandle(SQL_HANDLE_DBC, dbc);
        SQLFreeHandle(SQL_HANDLE_ENV, env);
        throw std::runtime_error("SQLExecDirectA failed");
    }

    QueryResult result;
    SQLSMALLINT col_count = 0;
    SQLNumResultCols(stmt, &col_count);

    for (SQLUSMALLINT i = 1; i <= col_count; ++i) {
        SQLCHAR col_name[256]{};
        SQLSMALLINT name_len = 0;
        SQLDescribeColA(stmt, i, col_name, static_cast<SQLSMALLINT>(sizeof(col_name)), &name_len, nullptr, nullptr, nullptr, nullptr);
        result.columns.emplace_back(reinterpret_cast<char*>(col_name));
    }

    while (SQLFetch(stmt) == SQL_SUCCESS) {
        std::vector<std::string> row;
        row.reserve(static_cast<std::size_t>(col_count));
        for (SQLUSMALLINT i = 1; i <= col_count; ++i) {
            SQLCHAR buffer[1024]{};
            SQLLEN indicator = 0;
            SQLGetData(stmt, i, SQL_C_CHAR, buffer, sizeof(buffer), &indicator);
            if (indicator == SQL_NULL_DATA) {
                row.emplace_back("");
            } else {
                row.emplace_back(reinterpret_cast<char*>(buffer));
            }
        }
        result.rows.push_back(std::move(row));
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    SQLDisconnect(dbc);
    SQLFreeHandle(SQL_HANDLE_DBC, dbc);
    SQLFreeHandle(SQL_HANDLE_ENV, env);
    return result;
#else
    (void)sql;
    throw std::runtime_error("SQL Server ODBC connector is only available on Windows.");
#endif
}

} // namespace sqlbot
