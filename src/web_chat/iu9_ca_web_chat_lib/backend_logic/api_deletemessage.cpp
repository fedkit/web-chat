#include "server_data_interact.h"
#include <engine_engine_number_9/baza_throw.h>
#include "../debug.h"

namespace iu9cawebchat {
    json::JSON internalapi_deleteMessage(SqliteConnection& conn, int64_t uid, const json::JSON& Sent) {
        int64_t chatId = Sent["chatUpdReq"]["chatId"].asInteger().get_int();
        int64_t my_role_here = get_role_of_user_in_chat(conn, uid, chatId);
        if (my_role_here == user_chat_role_deleted)
            een9_THROW("Unauthorized user tries to access internalapi_getChatInfo");
        if (my_role_here == user_chat_role_read_only)
            een9_THROW("read-only user can't send messages");

        int64_t LocalHistoryId = Sent["chatUpdReq"]["LocalHistoryId"].asInteger().get_int();
        int64_t msgId = Sent["id"].asInteger().get_int();
        RowMessage_Content msgInQuestion = lookup_message_content(conn, chatId, msgId);
        if (!(!msgInQuestion.isSystem && (msgInQuestion.senderUserId == uid || my_role_here == user_chat_role_admin) ))
            een9_THROW("Can't delete: permission denied");

        int64_t chat_HistoryId_BEFORE_EV = get_current_history_id_of_chat(conn, chatId);

        sqlite_nooutput(conn,
            "UPDATE `message` SET `exists` = 0, `text` = NULL, `chat_IncHistoryId` = ?1 WHERE `id` = ?2",
            {{1, chat_HistoryId_BEFORE_EV + 1}, {2, msgId}});

        sqlite_nooutput(conn, "UPDATE `chat` SET `it_HistoryId` = ?1 WHERE `id` = ?2",
            {{1, chat_HistoryId_BEFORE_EV + 1}, {2, chatId}}, {});

        json::JSON Recv;
        poll_update_chat(conn, Sent, Recv);
        return Recv;
    }
}
