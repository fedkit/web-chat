#include "server_data_interact.h"
#include <engine_engine_number_9/baza_throw.h>

namespace iu9cawebchat {
    void kick_from_chat(SqliteConnection& conn, int64_t chatId, int64_t alienUserId) {
        alter_user_chat_role(conn, chatId, alienUserId, user_chat_role_deleted);
    }

    json::JSON internalapi_removeMemberFromChat(SqliteConnection& conn, int64_t uid, const json::JSON& Sent) {
        int64_t chatId = Sent["chatUpdReq"]["chatId"].asInteger().get_int();
        int64_t my_role_here = get_role_of_user_in_chat(conn, uid, chatId);
        if (my_role_here != user_chat_role_admin)
            een9_THROW("Only admin can delete members of chat");
        int64_t badAlienId = Sent["userId"].asInteger().get_int();
        kick_from_chat(conn, chatId, badAlienId);
        insert_system_message_svo(conn, chatId, uid, "kicked", badAlienId);
        json::JSON Recv;
        poll_update_chat(conn, Sent, Recv);
        return Recv;
    }
}
