#include "sqlite3_wrapper.h"
#include "str_fields.h"
#include <engine_engine_number_9/baza_throw.h>
#include <assert.h>
#include <limits.h>

namespace iu9cawebchat {
    SqliteConnection::SqliteConnection(const std::string &file_path) {
        int ret = sqlite3_open(file_path.c_str(), &hand);
        if (ret != 0) {
            een9_THROW(std::string("Can't open sqlite3 database ") + sqlite3_errstr(ret));
        }
    }

    SqliteConnection::~SqliteConnection() {
        sqlite3_close_v2(hand);
    }

    void sqlite_nooutput(SqliteConnection& conn, const std::string& req_statement,
                                 const std::vector<std::pair<int, int64_t>>& int64_binds,
                                 const std::vector<std::pair<int, std::string>>& text8_binds) {
        SqliteStatement stmt(conn, req_statement, int64_binds, text8_binds);
        int ret;
        while (true) {
            ret = sqlite_stmt_step(stmt, {}, {});
            if (ret != SQLITE_ROW)
                break;
            int cc = sqlite3_column_count(stmt.stmt_obj);
            std::vector<int> types(cc);
            for (int i = 0; i < cc; i++)
                types[i] = sqlite3_column_type(stmt.stmt_obj, i);
            printf("Column: |");
            for (int i = 0; i < cc; i++) {
                switch (types[i]) {
#define ccase(tname) case SQLITE_ ## tname: printf(" " #tname " |"); break;
                    ccase(INTEGER)
                    ccase(FLOAT)
                    ccase(BLOB)
                    ccase(NULL)
                    case SQLITE3_TEXT:
                        printf(" TEXT |"); break;
                    default:
                        een9_THROW("AAAAAA");
                }
            }
            printf("\n");
            printf("Values: | ");
            for (int i = 0; i < cc; i++) {
                if (types[i] == SQLITE_INTEGER) {
                    printf("%lld | ", sqlite3_column_int64(stmt.stmt_obj, i));
                } else if (types[i] == SQLITE_FLOAT) {
                    printf("%lf | ", sqlite3_column_double(stmt.stmt_obj, i));
                } else if (types[i] == SQLITE_BLOB) {
                    const void* blob = sqlite3_column_blob(stmt.stmt_obj, i);
                    een9_ASSERT(sqlite3_errcode(conn.hand) == SQLITE_OK, "oom in sqlite3_column_blob");
                    size_t sz = sqlite3_column_bytes(stmt.stmt_obj, i);
                    printf("Blob of size %lu | ", sz);
                } else if (types[i] == SQLITE_NULL) {
                    printf("NULL | ");
                } else {
                    const unsigned char* text = sqlite3_column_text(stmt.stmt_obj, i);
                    een9_ASSERT(text, "oom in sqlite3_column_text");
                    printf("%s | ", (const char*)text);
                    // todo: print only if string is safe to print
                }
            }
            printf("\n");
        }
        printf("Request steps are done\n");
    }


    SqliteStatement::SqliteStatement(SqliteConnection &connection, const std::string &req_statement,
        const std::vector<std::pair<int, int64_t>> &int64_binds,
        const std::vector<std::pair<int, std::string>> &text8_binds): conn(connection) {

        int ret = sqlite3_prepare_v2(connection.hand, req_statement.c_str(), -1, &stmt_obj, NULL);
        if (ret != 0) {
            int err_pos = sqlite3_error_offset(connection.hand);
            een9_THROW("Compilation of request\n" + req_statement + "\nfailed" +
                ((err_pos >= 0) ? " with offset " + std::to_string(err_pos) : ""));
        }
        try {
            for (const std::pair<int, int64_t>& bv: int64_binds) {
                ret = sqlite3_bind_int64(stmt_obj, bv.first, bv.second);
                een9_ASSERT(ret == 0, "sqlite3_bind_int64");
            }
            for (const std::pair<int, std::string>& bv: text8_binds) {
                een9_ASSERT(is_orthodox_string(bv.second), "Can't bind this string to parameter");
                een9_ASSERT(bv.second.size() + 1 < INT_MAX, "Ah, oh, senpai, your string is toooo huge");
                ret = sqlite3_bind_text(stmt_obj, bv.first, bv.second.c_str(), (int)bv.second.size(), SQLITE_TRANSIENT);
                een9_ASSERT(ret == 0, "sqlite3_bind_text");
            }
        } catch (const std::exception& e) {
            sqlite3_finalize(stmt_obj);
            throw;
        }
    }

    SqliteStatement::~SqliteStatement() {
        sqlite3_finalize(stmt_obj);
    }

    void sqlite_stmt_bind_int64(SqliteStatement &stmt, int paramId, int64_t value) {
        int ret = sqlite3_bind_int64(stmt.stmt_obj, paramId, value);
        een9_ASSERT(ret == 0, "sqlite3_bind_int64");
    }

    int sqlite_stmt_step(SqliteStatement &stmt,
                         const std::vector<std::pair<int, fsql_integer_or_null *>> &ret_of_integer_or_null,
                         const std::vector<std::pair<int, fsql_text8_or_null *>> &ret_of_text8_or_null) {
        int ret = sqlite3_step(stmt.stmt_obj);
        if (ret == SQLITE_DONE)
            return ret;
        if (ret != SQLITE_ROW)
            een9_THROW(std::string("sqlite3_step ") + sqlite3_errstr(ret) + " :> " + sqlite3_errmsg(stmt.conn.hand));
        int cc = sqlite3_column_count(stmt.stmt_obj);
        for (auto& resp: ret_of_integer_or_null) {
            if (resp.first >= cc)
                een9_THROW("Not enough");
            int type = sqlite3_column_type(stmt.stmt_obj, resp.first);
            if (type == SQLITE_INTEGER) {
                *resp.second = fsql_integer_or_null{true, (int64_t)sqlite3_column_int64(stmt.stmt_obj, resp.first)};
            } else if (type == SQLITE_NULL) {
                *resp.second = fsql_integer_or_null{false, 0};
            } else
                een9_THROW("sqlite3_column_type. Incorrect type");
        }
        for (auto& resp: ret_of_text8_or_null) {
            if (resp.first >= cc)
                een9_THROW("Not enough");
            int type = sqlite3_column_type(stmt.stmt_obj, resp.first);
            if (type == SQLITE_TEXT) {
                /* Hm, yeah, I am reinterpret_casting between char and unsigned char again */
                const unsigned char* text = sqlite3_column_text(stmt.stmt_obj, resp.first);
                een9_ASSERT(text, "oom in sqlite3_column_text");
                *resp.second = fsql_text8_or_null{true, reinterpret_cast<const char*>(text)};
            } else if (type == SQLITE_NULL) {
                *resp.second = fsql_text8_or_null{false, ""};
            } else
                een9_THROW("sqlite3_column_type. Incorrect type");
        }
        return SQLITE_ROW;
    }

    int64_t sqlite_trsess_last_insert_rowid(SqliteConnection& conn) {
        int64_t res = sqlite3_last_insert_rowid(conn.hand);
        return res;
    }
}
