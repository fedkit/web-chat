#include "server_data_interact.h"
#include <engine_engine_number_9/baza_throw.h>
#include <assert.h>

namespace iu9cawebchat {
    bool is_membership_row_present(SqliteConnection& conn, int64_t chatId, int64_t alienUserId) {
        SqliteStatement req(conn,
            "SELECT EXISTS(SELECT 1 FROM `user_chat_membership` WHERE `chatId` = ?1 AND `userId` = ?2)",
            {{1, chatId}, {2, alienUserId}}, {});
        fsql_integer_or_null r{true, 0};
        int status = sqlite_stmt_step(req, {{0, &r}}, {});
        return (bool)r.value;
    }

    void alter_user_chat_role(SqliteConnection& conn, int64_t chatId, int64_t alienUserId, int64_t role) {
        int64_t chat_HistoryId_BEFORE_EV = get_current_history_id_of_chat(conn, chatId);
        int64_t alien_chatlist_HistoryId_BEFORE_EV = get_current_history_id_of_user_chatList(conn, alienUserId);
        if (!is_membership_row_present(conn, chatId, alienUserId)) {
            sqlite_nooutput(conn,
               "INSERT INTO `user_chat_membership` (`userId`, `chatId`, `user_chatList_IncHistoryId`,"
               "`chat_IncHistoryId`, `role`) VALUES (?1, ?2, ?3, ?4, ?5)",
               {{1, alienUserId}, {2, chatId}, {3, alien_chatlist_HistoryId_BEFORE_EV + 1},
               {4, chat_HistoryId_BEFORE_EV + 1}, {5, role}}, {});

        } else {
            sqlite_nooutput(conn,
                "UPDATE `user_chat_membership` SET `user_chatList_IncHistoryId` = ?3,`chat_IncHistoryId` = ?4,"
                "`role` = ?5 WHERE `userId` = ?1 AND `chatId` = ?2",
                {{1, alienUserId}, {2, chatId}, {3, alien_chatlist_HistoryId_BEFORE_EV + 1},
                {4, chat_HistoryId_BEFORE_EV + 1}, {5, role}}, {});
        }
        sqlite_nooutput(conn,
            "UPDATE `chat` SET `it_HistoryId` = ?1 WHERE `id` = ?2", {{1, chat_HistoryId_BEFORE_EV + 1},
            {2, chatId}}, {});

        sqlite_nooutput(conn,
            "UPDATE `user` SET `chatList_HistoryId` = ?1 WHERE `id` = ?2",
            {{1, alien_chatlist_HistoryId_BEFORE_EV + 1}, {2, alienUserId}}, {});
    }

    void make_her_a_member_of_the_midnight_crew(SqliteConnection& conn, int64_t chatId, int64_t alienUserId, int64_t role) {
        assert(role != user_chat_role_deleted);
        alter_user_chat_role(conn, chatId, alienUserId, role);
    }

    json::JSON internalapi_addMemberToChat(SqliteConnection& conn, int64_t uid, const json::JSON& Sent) {
        int64_t chatId = Sent["chatUpdReq"]["chatId"].asInteger().get_int();
        int64_t my_role_here = get_role_of_user_in_chat(conn, uid, chatId);
        if (my_role_here != user_chat_role_admin)
            een9_THROW("Non-admin user tries to access internalapi_getChatInfo");

        std::string alien_nickname = Sent["nickname"].asString();
        RowUser_Content alien;
        try {
            alien = lookup_user_content_by_nickname(conn, alien_nickname);
        } catch (std::exception& e) {
            return at_api_error_gen_bad_recv(-1l);
        }

        bool makeReadOnly = Sent["makeReadOnly"].toBool();

        int64_t aliens_old_role = get_role_of_user_in_chat(conn, alien.id, chatId);
        if (aliens_old_role == user_chat_role_deleted) {
            make_her_a_member_of_the_midnight_crew(conn, chatId, alien.id,
                makeReadOnly ? user_chat_role_read_only : user_chat_role_regular);
        } else {
            return at_api_error_gen_bad_recv(-2l);
        }
        insert_system_message_svo(conn, chatId, uid, "summoned", alien.id);

        json::JSON Recv;
        poll_update_chat(conn, Sent, Recv);
        return Recv;
    }
}
