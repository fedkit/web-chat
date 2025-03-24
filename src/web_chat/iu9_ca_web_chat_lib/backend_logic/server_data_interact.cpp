#include "server_data_interact.h"

#include <engine_engine_number_9/baza_throw.h>
#include <engine_engine_number_9/http_structures/cookies.h>
#include "../str_fields.h"

namespace iu9cawebchat {
    json::JSON at_api_error_gen_bad_recv(int64_t code) {
        return json::JSON(json::jdict{{"status", json::JSON(code)}});
    }
    
    const char* stringify_user_chat_role(int64_t role) {
        if (role == user_chat_role_admin)
            return "admin";
        if (role == user_chat_role_regular)
            return "regular";
        if (role == user_chat_role_read_only)
            return "read-only";
        return "not-a-member";
    }

    int64_t find_user_by_credentials (SqliteConnection& conn, const std::string& nickname, const std::string& password) {
        SqliteStatement sql_req(conn,
            "SELECT `id` FROM `user` WHERE `nickname` = ?1 AND `password` = ?2",
            {}, {{1, nickname}, {2, password}});
        fsql_integer_or_null id_col;
        int status = sqlite_stmt_step(sql_req, {{0, &id_col}}, {});
        if (status == SQLITE_ROW) {
            een9_ASSERT_pl(id_col.exist & id_col.value >= 0);
            return id_col.value;
        }
        return -1;
    }

    std::string get_user_name (SqliteConnection& conn, int64_t uid) {
        een9_ASSERT(uid >= 0, "Are you crazy?");
        SqliteStatement sql_req(conn,
           "SELECT `name` FROM `user` WHERE `id` = ?1",
           {{1, uid}}, {});
        fsql_text8_or_null name_col;
        int status = sqlite_stmt_step(sql_req, {}, {{0, &name_col}});
        if (status == SQLITE_ROW) {
            een9_ASSERT_pl(name_col.exist);
            return name_col.value;
        }
        een9_THROW("No such user");
    }

    RowUser_Content lookup_user_content(SqliteConnection &conn, int64_t uid) {
        een9_ASSERT(uid >= 0, "Are you crazy?");
        SqliteStatement sql_req(conn,
           "SELECT `nickname`, `name` FROM `user` WHERE `id` = ?1",
           {{1, uid}}, {});
        fsql_text8_or_null nickname_col;
        fsql_text8_or_null name_col;
        int status = sqlite_stmt_step(sql_req, {}, {{0, &nickname_col}, {1, &name_col}});
        if (status == SQLITE_ROW) {
            return {uid, std::move(nickname_col.value), std::move(name_col.value)};
        }
        een9_THROW("No such user");
    }

    RowUser_Content lookup_user_content_by_nickname(SqliteConnection& conn, const std::string& nickname) {
        SqliteStatement sql_req(conn,
           "SELECT `id`, `name` FROM `user` WHERE `nickname` = ?1",
           {}, {{1, nickname}});
        fsql_integer_or_null id_col;
        fsql_text8_or_null name_col;
        int status = sqlite_stmt_step(sql_req, {{0, &id_col}}, {{1, &name_col}});
        if (status == SQLITE_ROW) {
            return {id_col.value, nickname, std::move(name_col.value)};
        }
        een9_THROW("No such user");
    }

    RowChat_Content lookup_chat_content(SqliteConnection &conn, int64_t chatId) {
        een9_ASSERT(chatId >= 0, "Are you crazy?");
        SqliteStatement sql_req(conn,
           "SELECT `nickname`, `name`, `lastMsgId` FROM `chat` WHERE `id` = ?1",
           {{1, chatId}}, {});
        fsql_text8_or_null nickname_col;
        fsql_text8_or_null name_col;
        fsql_integer_or_null last_msg_id_col;
        int status = sqlite_stmt_step(sql_req, {{2, &last_msg_id_col}}, {{0, &nickname_col}, {1, &name_col}});
        if (status == SQLITE_ROW) {
            return {chatId, std::move(nickname_col.value), std::move(name_col.value),
                last_msg_id_col.exist ? last_msg_id_col.value : -1};
        }
        een9_THROW("No such chat");
    }

    RowChat_Content lookup_chat_content_by_nickname(SqliteConnection &conn, const std::string& nickname) {
        SqliteStatement sql_req(conn,
           "SELECT `id`, `name`, `lastMsgId` FROM `chat` WHERE `nickname` = ?1",
           {}, {{1, nickname}});
        fsql_integer_or_null id_col;
        fsql_text8_or_null name_col;
        fsql_integer_or_null last_msg_id_col;
        int status = sqlite_stmt_step(sql_req, {{0, &id_col}, {2, &last_msg_id_col}}, {{1, &name_col}});
        if (status == SQLITE_ROW) {
            return {id_col.value, nickname, std::move(name_col.value),
                last_msg_id_col.exist ? last_msg_id_col.value : -1};
        }
        een9_THROW("No such chat");
    }

    RowMessage_Content lookup_message_content(SqliteConnection& conn, int64_t chatId, int64_t msgId) {
        SqliteStatement req(conn,
            "SELECT `senderUserId`, `exists`, `isSystem`, `text` FROM `message` WHERE "
            "`chatId` = ?1 AND `id` = ?2", {{1, chatId}, {2, msgId}}, {});
        fsql_integer_or_null senderUserId, exists, isSystem;
        fsql_text8_or_null msg_text;
        int status = sqlite_stmt_step(req, {{0, &senderUserId}, {1, &exists}, {2, &isSystem}},
            {{3, &msg_text}});
        if (status == SQLITE_ROW) {
            return {msgId, senderUserId.exist ? senderUserId.value : -1, (bool)exists.value,
                (bool)isSystem.value, msg_text.value};
        }
        een9_THROW("No such message");
    }

    int64_t get_role_of_user_in_chat(SqliteConnection& conn, int64_t userId, int64_t chatId) {
        SqliteStatement req(conn,
            "SELECT `role` FROM `user_chat_membership` WHERE `userId` = ?1 AND `chatId` = ?2",
            {{1, userId}, {2, chatId}}, {});
        fsql_integer_or_null role;
        int status = sqlite_stmt_step(req, {{0, &role}}, {});
        if (status == SQLITE_ROW) {
            return role.exist ? (int)role.value : user_chat_role_deleted;
        }
        return user_chat_role_deleted;
    }

    int64_t get_lastMsgId_of_chat(SqliteConnection &conn, int64_t chatId) {
        een9_ASSERT(chatId >= 0, "Are you crazy?");
        SqliteStatement sql_req(conn,
           "SELECT `lastMsgId` FROM `chat` WHERE `id` = ?1", {{1, chatId}}, {});
        fsql_integer_or_null last_msg_id_col;
        int status = sqlite_stmt_step(sql_req, {{0, &last_msg_id_col}}, {});
        if (status == SQLITE_ROW) {
            return last_msg_id_col.exist ? last_msg_id_col.value : -1;
        }
        een9_THROW("No such chat");
    }

    /* All the api calls processing is done in dedicated files.
     * All functions related to polling are defined in api_pollevents.cpp */

    int64_t get_current_history_id_of_chat(SqliteConnection& conn, int64_t chatId) {
        SqliteStatement req(conn, "SELECT `it_HistoryId` FROM `chat` WHERE `id` = ?1", {{1, chatId}}, {});
        fsql_integer_or_null HistoryId;
        int status = sqlite_stmt_step(req, {{0, &HistoryId}}, {});
        een9_ASSERT_pl(status == SQLITE_ROW);
        return HistoryId.value;
    }

    int64_t get_current_history_id_of_user_chatList(SqliteConnection& conn, int64_t userId) {
        SqliteStatement req(conn, "SELECT `chatList_HistoryId` FROM `user` WHERE `id` = ?1", {{1, userId}}, {});
        fsql_integer_or_null HistoryId;
        int status = sqlite_stmt_step(req, {{0, &HistoryId}}, {});
        een9_ASSERT_pl(status == SQLITE_ROW);
        return HistoryId.value;
    }


    // todo: extract useful clues from deprecated code.
    // todo: deprecated code goes here:
    /* !!! DEPRECATED FUNCTION */
    json::JSON toremoveinternalapi_getChatList(SqliteConnection& conn, int64_t uid, const json::JSON& Sent) {
        json::JSON Recv;
        Recv["status"] = json::JSON(0l);
        Recv["chats"] = json::JSON(json::array);
        std::vector<json::JSON>& chats = Recv["chats"].asArray();
        SqliteStatement req(conn,
            "SELECT `chat`.`id`, `chat`.`nickname`, `chat`.`name`, `chat`.`lastMsgId`, "
            "`user_chat_membership`.`role` FROM `chat` "
            "RIGHT JOIN `user_chat_membership` ON `chat`.`id` = `user_chat_membership`.`chatId` "
            "WHERE `user_chat_membership`.`userId` = ?1 ", {{1, uid}}, {});
        while (true) {
            fsql_integer_or_null chat_id;
            fsql_text8_or_null chat_nickname, chat_name;
            fsql_integer_or_null chat_lastMsgId, role_here;
            int status = sqlite_stmt_step(req, {{0, &chat_id}, {3, &chat_lastMsgId}, {4, &role_here}},
                {{1, &chat_nickname}, {2, &chat_name}});
            if (status != SQLITE_ROW)
                break;
            chats.emplace_back();
            json::JSON& chat = chats.back();
            chat["id"] = json::JSON(chat_id.value);
            chat["content"]["nickname"] = json::JSON(chat_nickname.value);
            chat["content"]["name"] = json::JSON(chat_name.value);
            chat["content"]["lastMsgId"] = json::JSON(chat_lastMsgId.exist ? chat_lastMsgId.value : -1);
            chat["content"]["roleHere"] = json::JSON(stringify_user_chat_role(role_here.value));
        }
        return Recv;
    }

    /* !!! DEPRECATED FUNCTION */
    json::JSON toremoveinternalapi_getChatMemberList(SqliteConnection& conn, int64_t uid, const json::JSON& Sent) {
        int64_t chatId = Sent["id"].asInteger().get_int();
        int64_t my_role_here = get_role_of_user_in_chat(conn, uid, chatId);
        if (my_role_here == user_chat_role_deleted)
            een9_THROW("Unauthorized user tries to access internalapi_getChatInfo");
        json::JSON Recv;
        Recv["status"] = json::JSON(0l);
        Recv["members"] = json::JSON(json::array);
        std::vector<json::JSON>& members = Recv["members"].asArray();
        SqliteStatement req(conn,
            "SELECT `user`.`id`, `user`.`nickname`, `user`.`name`, `user_chat_membership`.`role` FROM "
            "`user` RIGHT JOIN `user_chat_membership` ON `user`.`id` = `user_chat_membership`.`userId` "
            "WHERE `user_chat_membership`.`chatId` = ?1",
            {{1, chatId}}, {});
        while (true) {
            fsql_integer_or_null this_user_id;
            fsql_text8_or_null this_user_nickname, this_user_name;
            fsql_integer_or_null this_users_role;
            int status = sqlite_stmt_step(req, {{0, &this_user_id}, {3, &this_users_role}},
                {{1, &this_user_nickname}, {2, &this_user_name}});
            if (status != SQLITE_ROW)
                break;
            members.emplace_back();
            json::JSON& member = members.back();
            member["id"] = json::JSON(this_user_id.value);
            member["content"]["nickname"] = json::JSON(this_user_nickname.value);
            member["content"]["name"] = json::JSON(this_user_name.value);
            member["content"]["role"] = json::JSON(this_users_role.value);
        }
        return Recv;
    }

    bool is_nickname_taken(SqliteConnection& conn, const std::string& nickname) {
        if (!check_nickname(nickname))
            return true;
        SqliteStatement req(conn, "SELECT EXISTS(SELECT 1 FROM `nickname` WHERE `it` = ?1)",
            {}, {{1, nickname}});
        fsql_integer_or_null r{true, 0};
        int status = sqlite_stmt_step(req, {{0, &r}}, {});
        return r.value;
    }

    void reserve_nickname(SqliteConnection& conn, const std::string& nickname) {
        if (!check_nickname(nickname))
            een9_THROW("PRECAUTION! Trying to insert incorrect nickname into nickname table");
        sqlite_nooutput(conn, "INSERT INTO `nickname` (`it`) VALUES (?1)", {}, {{1, nickname}});
    }
}
