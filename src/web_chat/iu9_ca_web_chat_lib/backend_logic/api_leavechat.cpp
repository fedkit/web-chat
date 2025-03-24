#include "server_data_interact.h"
#include <engine_engine_number_9/baza_throw.h>

namespace iu9cawebchat {
    json::JSON internalapi_leaveChat(SqliteConnection& conn, int64_t uid, const json::JSON& Sent) {
        int64_t chatId = Sent["chatId"].asInteger().get_int();
        if (get_role_of_user_in_chat(conn, uid, chatId) == user_chat_role_deleted)
            een9_THROW("Not a member");
        kick_from_chat(conn, chatId, uid);
        insert_system_message_svo(conn, chatId, uid, "left", -1);
        json::JSON Recv;
        poll_update_chat_list(conn, uid, Sent, Recv);
        return Recv;
    }
}
