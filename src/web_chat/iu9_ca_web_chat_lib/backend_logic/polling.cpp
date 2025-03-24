#include "server_data_interact.h"
#include <assert.h>
#include <engine_engine_number_9/baza_throw.h>
#include "../debug.h"

namespace iu9cawebchat {
    json::JSON poll_update_chat_list_resp(SqliteConnection& conn, int64_t userId, int64_t LocalHistoryId) {
        printf("Userid: %ld\n", userId);
        json::JSON chatListUpdResp;
        SqliteStatement my_membership_changes(conn,
           "SELECT `chatId`, `role` FROM `user_chat_membership` WHERE `userId` = ?1 "
           "AND `user_chatList_IncHistoryId` > ?2", {{1, userId}, {2, LocalHistoryId}});
        json::jarr& myChats = chatListUpdResp["myChats"].asArray();
        while (true) {
            fsql_integer_or_null ev_chatId, usersRoleHere;
            int status = sqlite_stmt_step(my_membership_changes, {{0, &ev_chatId}, {1, &usersRoleHere}}, {});
            if (status != SQLITE_ROW)
                break;
            myChats.emplace_back();
            json::JSON& myMembershipSt = myChats.back();
            myMembershipSt["chatId"].asInteger() = json::Integer(ev_chatId.value);
            myMembershipSt["myRoleHere"].asString() = stringify_user_chat_role(usersRoleHere.value);
            if (usersRoleHere.value != user_chat_role_deleted) {
                RowChat_Content CHAT = lookup_chat_content(conn, ev_chatId.value);
                myMembershipSt["chatName"].asString() = CHAT.name;
                myMembershipSt["chatNickname"].asString() = CHAT.nickname;
            }
        }
        int64_t HistoryId = get_current_history_id_of_user_chatList(conn, userId);
        chatListUpdResp["HistoryId"].asInteger() = json::Integer(HistoryId);
        return chatListUpdResp;
    }

    void poll_update_chat_list(SqliteConnection& conn, int64_t userId, const json::JSON& Sent, json::JSON& Recv) {
        Recv["status"].asInteger() = json::Integer(0l);
        // todo: in libjsonincpp: get rid of Integer
        Recv["chatListUpdResp"] = poll_update_chat_list_resp(conn, userId, Sent["chatListUpdReq"]["LocalHistoryId"].asInteger().get_int());
    }

    json::JSON make_messageSt_obj(int64_t id, int64_t senderUserId, bool exists, bool isSystem, const std::string& text) {
        json::JSON messageSt;
        messageSt["id"].asInteger() = json::Integer(id);
        if (!isSystem)
            messageSt["senderUserId"].asInteger() = json::Integer(senderUserId);
        messageSt["exists"] = json::JSON(exists);
        messageSt["isSystem"] = json::JSON(isSystem);
        if (exists)
            messageSt["text"].asString() = text;
        return messageSt;
    }

    json::jarr poll_update_chat_resp_messages(SqliteConnection& conn, int64_t chatId, int64_t LocalHistoryId,
        int64_t QSEG_A, int64_t QSEG_B) {

        json::jarr messages;
        SqliteStatement messages_changes(conn,
            "SELECT `id`, `senderUserId`, `exists`, `isSystem`, `text` FROM `message` "
            "WHERE `chatId` = ?1 AND ( `chat_IncHistoryId` > ?2 OR ( ?3 <= `id` AND `id` <= ?4 ) )",
            {{1, chatId}, {2, LocalHistoryId}, {3, QSEG_A}, {4, QSEG_B}}, {});
        while (true) {
            fsql_integer_or_null msgId, msgSenderUserId, msgExists, msgIsSystem;
            fsql_text8_or_null msgText;
            int status = sqlite_stmt_step(messages_changes,
                {{0, &msgId}, {1, &msgSenderUserId}, {2, &msgExists}, {3, &msgIsSystem}},
                {{4, &msgText}});
            if (status != SQLITE_ROW)
                break;
            messages.push_back(make_messageSt_obj(msgId.value, msgSenderUserId.value, msgExists.value,
                msgIsSystem.value, msgText.value));
        }
        return messages;
    }

    json::jarr poll_update_chat_resp_members(SqliteConnection& conn, int64_t chatId, int64_t LocalHistoryId) {
        json::jarr members;

        SqliteStatement membership_changes(conn,
                   "SELECT `userId`, `role` FROM `user_chat_membership` WHERE `chatId` = ?1 "
                   "AND `chat_IncHistoryId` > ?2", {{1, chatId}, {2, LocalHistoryId}}, {});
        while (true) {
            fsql_integer_or_null alienUserId;
            fsql_integer_or_null alienRoleHere;
            int status = sqlite_stmt_step(membership_changes,
                {{0, &alienUserId}, {1, &alienRoleHere}}, {});
            if (status != SQLITE_ROW)
                break;
            members.emplace_back();
            json::JSON& memberSt = members.back();
            memberSt["userId"].asInteger() = json::Integer(alienUserId.value);
            memberSt["roleHere"].asString() = stringify_user_chat_role(alienRoleHere.value);
            if (alienRoleHere.value != user_chat_role_deleted) {
                RowUser_Content alien = lookup_user_content(conn, alienUserId.value);
                memberSt["name"].asString() = alien.name;
                memberSt["nickname"].asString() = alien.nickname;
            }
        }
        return members;
    }

    json::JSON poll_update_chat_ONE_MSG_resp(SqliteConnection& conn, int64_t chatId, int64_t selectedMsg) {
        json::JSON chatUpdResp;

        chatUpdResp["members"].asArray() = poll_update_chat_resp_members(conn, chatId, 0);


        json::jarr& messages = chatUpdResp["messages"].asArray();
        if (selectedMsg >= 0) {
            RowMessage_Content msg = lookup_message_content(conn, chatId, selectedMsg);
            messages.push_back(make_messageSt_obj(msg.id, msg.senderUserId, msg.exists, msg.isSystem, msg.text));
        }

        int64_t lastMsgId = get_lastMsgId_of_chat(conn, chatId);
        chatUpdResp["lastMsgId"].asInteger() = json::Integer(lastMsgId);

        int64_t HistoryId = get_current_history_id_of_chat(conn, chatId);
        chatUpdResp["HistoryId"].asInteger() = json::Integer(HistoryId);
        return chatUpdResp;
    }

    json::JSON poll_update_chat_important_segment_resp(SqliteConnection& conn, int64_t chatId, int64_t LocalHistoryId,
        int64_t QSEG_A, int64_t QSEG_B) {

        json::JSON chatUpdResp;

        chatUpdResp["members"].asArray() = poll_update_chat_resp_members(conn, chatId, LocalHistoryId);
        chatUpdResp["messages"].asArray() = poll_update_chat_resp_messages(conn, chatId, LocalHistoryId, QSEG_A, QSEG_B);

        int64_t lastMsgId = get_lastMsgId_of_chat(conn, chatId);
        chatUpdResp["lastMsgId"].asInteger() = json::Integer(lastMsgId);

        int64_t HistoryId = get_current_history_id_of_chat(conn, chatId);
        chatUpdResp["HistoryId"].asInteger() = json::Integer(HistoryId);
        return chatUpdResp;
    }

    /* chat polling function MUST have one queer feature: it accepts a range of msgId, which are guaranteed to be
     * lookud up. */
    void poll_update_chat_important_segment(SqliteConnection& conn, const json::JSON& Sent, json::JSON& Recv,
        int64_t QSEG_A, int64_t QSEG_B) {

        Recv["status"].asInteger() = json::Integer(0l);
        Recv["chatUpdResp"] = poll_update_chat_important_segment_resp(conn,
            Sent["chatUpdReq"]["chatId"].asInteger().get_int(),
            Sent["chatUpdReq"]["LocalHistoryId"].asInteger().get_int(), QSEG_A, QSEG_B);
    }

    void poll_update_chat(SqliteConnection& conn, const json::JSON& Sent, json::JSON& Recv) {
        poll_update_chat_important_segment(conn, Sent, Recv, -1, -2);
    }

    json::JSON internalapi_chatPollEvents(SqliteConnection& conn, int64_t uid, const json::JSON& Sent) {
        int64_t chatId = Sent["chatUpdReq"]["chatId"].asInteger().get_int();
        if (get_role_of_user_in_chat(conn, uid, chatId) == user_chat_role_deleted)
            een9_THROW("chatPollEvents: trying to access chat that user does not belong to");
        json::JSON Recv;
        poll_update_chat(conn, Sent, Recv);
        return Recv;
    }

    json::JSON internalapi_chatListPollEvents(SqliteConnection& conn, int64_t uid, const json::JSON& Sent) {
        json::JSON Recv;
        poll_update_chat_list(conn, uid, Sent, Recv);
        return Recv;
    }


    /* Reznya */
    json::JSON internalapi_getMessageNeighbours(SqliteConnection& conn, int64_t uid, const json::JSON& Sent) {
        int64_t chatId = Sent["chatUpdReq"]["chatId"].asInteger().get_int();
        if (get_role_of_user_in_chat(conn, uid, chatId) == user_chat_role_deleted)
            een9_THROW("Authentication failure");
        int64_t lastMsgId = get_lastMsgId_of_chat(conn, chatId);
        bool dir_forward = Sent["direction"].asString() == "forward";
        int64_t amount = Sent["amount"].asInteger().get_int();
        int64_t K = Sent["msgId"].asInteger().get_int();
        if (amount <= 0 || amount > 15)
            een9_THROW("Incorrect amount");
        json::JSON Recv;
        int64_t qBeg = -1;
        int64_t qEnd = -2;
        if (lastMsgId >= 0) {
            if (K < 0) {
                if (dir_forward)
                    een9_THROW("Can't go from the top of chat");
                qBeg = std::max(0l, lastMsgId - amount + 1);
                qEnd = lastMsgId;
            } else if (dir_forward) {
                qBeg = K + 1;
                qEnd = K + amount;
            } else {
                qBeg = K - amount;
                qEnd = K - 1;
            }
        }
        poll_update_chat_important_segment(conn, Sent, Recv, qBeg, qEnd);
        return Recv;
    }
}
