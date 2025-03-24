#include "actions.h"
#include <engine_engine_number_9/baza_throw.h>
#include "str_fields.h"
#include <sqlite3.h>
#include <engine_engine_number_9/os_utils.h>
#include "find_db.h"
#include <unistd.h>
#include <assert.h>
#include "sqlite3_wrapper.h"
#include "backend_logic/server_data_interact.h"

namespace iu9cawebchat {
    void initialize_website(const json::JSON& config, const std::string& root_pw) {
        printf("Initialization...\n");
        if (!check_strong_password(root_pw))
            een9_THROW("Bad root password");
        std::string db_path;
        int ret;
        ret = find_db_sqlite_file_path(config, db_path);
        if (ret != 0)
            een9_THROW("Invalid settings[\"database\"] field");
        if (een9::isRegularFile(db_path)) {
            // todo: plaese, don't do this
            ret = unlink(db_path.c_str());
            if (ret != 0)
                een9_THROW("unlink");
        }
        if (een9::isRegularFile(db_path))
            een9_THROW("Database file exists prior to initialization. Can't preceed withut harming existing data");
        SqliteConnection conn(db_path.c_str());
        /* Role of memeber of chat:
         * 1 - admin
         * 2 - regular
         * 3 - read-only member
         * 4 - deleted (no longer a member)
         *
         * If user.id is 0, it is a root user
         * If chat.lastMsgId is NULL, chat is empty
         * If message.previous is NULL, this message is first in it's chat
         */
        try {
            sqlite_nooutput(conn, "PRAGMA foreign_keys = true");
            sqlite_nooutput(conn, "BEGIN");
            sqlite_nooutput(conn, "CREATE TABLE `nickname` (`it` TEXT PRIMARY KEY NOT NULL) WITHOUT ROWID");
            sqlite_nooutput(conn, "CREATE TABLE `user` ("
                                             "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
                                             "`nickname` TEXT UNIQUE REFERENCES `nickname` NOT NULL,"
                                             "`name` TEXT NOT NULL,"
                                             "`chatList_HistoryId` INTEGER NOT NULL,"
                                             "`password` TEXT NOT NULL,"
                                             "`bio` TEXT NOT NULL"
                                             ")");
            sqlite_nooutput(conn, "CREATE TABLE `chat` ("
                                             "`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
                                             "`nickname` TEXT UNIQUE REFERENCES `nickname` NOT NULL,"
                                             "`name` TEXT NOT NULL,"
                                             "`it_HistoryId` INTEGER NOT NULL,"
                                             "`lastMsgId` INTEGER NOT NULL"
                                             ")");
            sqlite_nooutput(conn, "CREATE TABLE `user_chat_membership` ("
                                             "`userId` INTEGER REFERENCES `user` NOT NULL,"
                                             "`chatId` INTEGER REFERENCES `chat` NOT NULL,"
                                             "`user_chatList_IncHistoryId` INTEGER NOT NULL,"
                                             "`chat_IncHistoryId` INTEGER NOT NULL,"
                                             "`role` INTEGER NOT NULL,"
                                             "UNIQUE (`userId`, `chatId`)"
                                             ")");
            sqlite_nooutput(conn, "CREATE TABLE `message` ("
                                             "`chatId` INTEGER REFERENCES `chat` NOT NULL,"
                                             "`id` INTEGER NOT NULL,"
                                             "`senderUserId` INTEGER REFERENCES `user`,"
                                             "`exists` BOOLEAN NOT NULL,"
                                             "`isSystem` BOOLEAN NOT NULL,"
                                             "`text` TEXT,"
                                             "`chat_IncHistoryId` INTEGER NOT NULL,"
                                             "PRIMARY KEY (`chatId`, `id`)"
                                             ")");
            std::vector<std::string> sus = {"unknown", "undefined", "null", "none", "None", "NaN"};
            for (auto& s: sus)
                reserve_nickname(conn, s);
            add_user(conn, "root", "Rootov Root Rootovich", root_pw, "One admin to rule them all", 0);
            sqlite_nooutput(conn, "END");
        } catch (const std::exception& e) {
            sqlite_nooutput(conn, "ROLLBACK", {}, {});
            throw;
        }
    }
}
