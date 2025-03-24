#ifndef IU9_CA_WEB_CHAT_SERVICE_SQLITE_WRAP_H
#define IU9_CA_WEB_CHAT_SERVICE_SQLITE_WRAP_H

#include <sqlite3.h>
#include <vector>
#include <string>

namespace iu9cawebchat {
    struct SqliteConnection {
        sqlite3* hand = NULL;
        SqliteConnection() = default;
        explicit SqliteConnection(const std::string& file_path);
        SqliteConnection (const SqliteConnection&) = delete;
        SqliteConnection& operator= (const SqliteConnection&) = delete;
        ~SqliteConnection();
    };

    void sqlite_nooutput(SqliteConnection& conn, const std::string& req_statement,
                                 const std::vector<std::pair<int, int64_t>>& int64_binds = {},
                                 const std::vector<std::pair<int, std::string>>& text8_binds = {});

    struct fsql_integer_or_null {
        bool exist;
        int64_t value;
    };

    struct fsql_text8_or_null {
        bool exist;
        std::string value;
    };

    struct SqliteStatement {
        SqliteConnection& conn;
        sqlite3_stmt* stmt_obj = NULL;
        SqliteStatement(SqliteConnection& connection, const std::string& req_statement,
            const std::vector<std::pair<int, int64_t>>& int64_binds = {},
            const std::vector<std::pair<int, std::string>>& text8_binds = {});
        SqliteStatement(SqliteStatement&) = delete;
        SqliteStatement& operator=(SqliteStatement&) = delete;

        ~SqliteStatement();
    };

    void sqlite_stmt_bind_int64(SqliteStatement& stmt, int paramId, int64_t value);

    int sqlite_stmt_step(SqliteStatement& stmt,
        const std::vector<std::pair<int, fsql_integer_or_null*>>& ret_of_integer_or_null,
        const std::vector<std::pair<int, fsql_text8_or_null*>>& ret_of_text8_or_null);

    int64_t sqlite_trsess_last_insert_rowid(SqliteConnection& conn);
}

#endif
