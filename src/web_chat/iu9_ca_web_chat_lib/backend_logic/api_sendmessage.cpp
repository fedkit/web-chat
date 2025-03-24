#include "server_data_interact.h"
#include <engine_engine_number_9/baza_throw.h>
#include "../str_fields.h"
#include "../debug.h"

namespace iu9cawebchat {
    /* No authorization check is performed
     * Chat's HistoryId will increment after this operation
     * if adding system message, uid is ignored
     */
    void insert_new_message(SqliteConnection& conn, int64_t uid, int64_t chatId, const std::string& text, bool isSystem) {
        int64_t chat_HistoryId_BEFORE_MSG = get_current_history_id_of_chat(conn, chatId);
        int64_t chat_lastMsgId = get_lastMsgId_of_chat(conn, chatId);
        SqliteStatement req(conn,
            "INSERT INTO `message` (`chatId`, `id`, `senderUserId`, `exists`, `isSystem`, `chat_IncHistoryId`, "
            "`text`) VALUES (?1, ?2, ?3, 1, ?4, ?5, ?6)",
            {{1, chatId}, {2, chat_lastMsgId + 1}, {4, (int64_t)isSystem}, {5, chat_HistoryId_BEFORE_MSG + 1}}, {{6, text}});
        if (!isSystem)
            sqlite_stmt_bind_int64(req, 3, uid);
        if (sqlite_stmt_step(req, {}, {}) != SQLITE_DONE)
            een9_THROW("There must be something wrong");
        sqlite_nooutput(conn, "UPDATE `chat` SET `lastMsgId` = ?1, `it_HistoryId` = ?2 WHERE `id` = ?3",
            {{1, chat_lastMsgId + 1}, {2, chat_HistoryId_BEFORE_MSG + 1}, {3, chatId}}, {});
    }

    void insert_system_message_svo(SqliteConnection& conn, int64_t chatId,
        int64_t subject, const std::string& verb, int64_t object) {

        insert_new_message(conn, -1, chatId,
            std::to_string(subject) + "," + verb + "," + std::to_string(object), true);
    }

    json::JSON internalapi_sendMessage(SqliteConnection& conn, int64_t uid, const json::JSON& Sent) {
        int64_t chatId = Sent["chatUpdReq"]["chatId"].asInteger().get_int();
        int64_t my_role_here = get_role_of_user_in_chat(conn, uid, chatId);
        if (my_role_here == user_chat_role_deleted)
            een9_THROW("Unauthorized user tries to access internalapi_getChatInfo");
        if (my_role_here == user_chat_role_read_only)
            een9_THROW("read-only user can't send messages");

        std::string text = Sent["content"]["text"].asString();
        if (!is_orthodox_string(text) || text.empty())
            een9_THROW("Bad input text");
        insert_new_message(conn, uid, chatId, text, false);

        json::JSON Recv;
        poll_update_chat(conn, Sent, Recv);
        return Recv;
    }
}
