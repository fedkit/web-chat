#include "server_data_interact.h"
#include <engine_engine_number_9/baza_throw.h>
#include "../str_fields.h"

namespace iu9cawebchat {
    json::JSON internalapi_createChat(SqliteConnection& conn, int64_t uid, const json::JSON& Sent) {
        std::string new_chat_name = Sent["content"]["name"].asString();
        std::string new_chat_nickname = Sent["content"]["nickname"].asString();
        if (!check_nickname(new_chat_nickname) || !check_name(new_chat_name))
            return at_api_error_gen_bad_recv(-1l);
        if (is_nickname_taken(conn, new_chat_nickname))
            return at_api_error_gen_bad_recv(-2l);
        if (is_nickname_taken(conn, new_chat_nickname))
            return at_api_error_gen_bad_recv(-3l);
        reserve_nickname(conn, new_chat_nickname);

        sqlite_nooutput(conn,
            "INSERT INTO `chat` (`nickname`, `name`, `it_HistoryId`, `lastMsgId`) VALUES (?1, ?2, 0, -1)",
            {}, {{1, new_chat_nickname}, {2, new_chat_name}});

        int64_t CHAT_ID = sqlite_trsess_last_insert_rowid(conn);

        make_her_a_member_of_the_midnight_crew(conn, CHAT_ID, uid, user_chat_role_admin);

        // todo: send a message into chat
        json::JSON Recv;
        poll_update_chat_list(conn, uid, Sent, Recv);
        return Recv;
    }
}
