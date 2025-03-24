#ifndef IU9_CA_WEB_CHAT_LIB_BACKEND_LOGIC_SERVER_DATA_INTERACT_H
#define IU9_CA_WEB_CHAT_LIB_BACKEND_LOGIC_SERVER_DATA_INTERACT_H

/* This folder covers all code that helps to interact with database and templater,
*  or dictates the logic of how this web chat functions */

#include "../sqlite3_wrapper.h"
#include <jsonincpp/string_representation.h>

namespace iu9cawebchat {
    json::JSON at_api_error_gen_bad_recv(int64_t code = -1);

    constexpr int64_t user_chat_role_admin = 1;
    constexpr int64_t user_chat_role_regular = 2;
    constexpr int64_t user_chat_role_read_only = 3;
    constexpr int64_t user_chat_role_deleted = 4;

    const char* stringify_user_chat_role(int64_t role);

    int64_t find_user_by_credentials (SqliteConnection& conn, const std::string& nickname, const std::string& password);
    std::string get_user_name (SqliteConnection& conn, int64_t uid);

    struct RowUser_Content {
        int64_t id;
        std::string nickname;
        std::string name;
    };

    struct RowChat_Content {
        int64_t id;
        std::string nickname;
        std::string name;
        int64_t lastMsgId;  // Negative if it does not exist
    };

    RowUser_Content lookup_user_content(SqliteConnection& conn, int64_t uid);
    RowUser_Content lookup_user_content_by_nickname(SqliteConnection& conn, const std::string& nickname);
    /* Does not make authorization check */
    RowChat_Content lookup_chat_content(SqliteConnection& conn, int64_t chatId);
    RowChat_Content lookup_chat_content_by_nickname(SqliteConnection &conn, const std::string& nickname);

    struct RowMessage_Content {
        int64_t id;
        int64_t senderUserId;
        bool exists;
        bool isSystem;
        std::string text;
    };

    RowMessage_Content lookup_message_content(SqliteConnection& conn, int64_t chatId, int64_t msgId);

    /* Does not make authorization check */
    int64_t get_role_of_user_in_chat(SqliteConnection& conn, int64_t userId, int64_t chatId);
    /* Does not make authorization check */
    int64_t get_lastMsgId_of_chat(SqliteConnection& conn, int64_t chatId);

    int64_t get_current_history_id_of_chat(SqliteConnection& conn, int64_t chatId);
    int64_t get_current_history_id_of_user_chatList(SqliteConnection& conn, int64_t userId);

    json::JSON poll_update_chat_list_resp(SqliteConnection& conn, int64_t userId, int64_t LocalHistoryId);
    void poll_update_chat_list(SqliteConnection& conn, int64_t userId, const json::JSON& Sent, json::JSON& Recv);

    json::JSON poll_update_chat_ONE_MSG_resp(SqliteConnection& conn, int64_t chatId, int64_t selectedMsg);
    void poll_update_chat(SqliteConnection& conn, const json::JSON& Sent, json::JSON& Recv);

    void alter_user_chat_role(SqliteConnection& conn, int64_t chatId, int64_t alienUserId, int64_t role);
    void make_her_a_member_of_the_midnight_crew(SqliteConnection& conn, int64_t chatId, int64_t alienUserId, int64_t role);
    void kick_from_chat(SqliteConnection& conn, int64_t chatId, int64_t alienUserId);

    bool is_nickname_taken(SqliteConnection& conn, const std::string& nickname);
    void reserve_nickname(SqliteConnection& conn, const std::string& nickname);

    void insert_new_message(SqliteConnection& conn, int64_t uid, int64_t chatId, const std::string& text, bool isSystem);
    void insert_system_message_svo(SqliteConnection& conn, int64_t chatId,
        int64_t subject, const std::string& verb, int64_t object);

    /* ============================= API ==================================== */
    json::JSON internalapi_chatPollEvents(SqliteConnection& conn, int64_t uid, const json::JSON& Sent);
    json::JSON internalapi_chatListPollEvents(SqliteConnection& conn, int64_t uid, const json::JSON& Sent);
    json::JSON internalapi_getMessageNeighbours(SqliteConnection& conn, int64_t uid, const json::JSON& Sent);
    json::JSON internalapi_sendMessage(SqliteConnection& conn, int64_t uid, const json::JSON& Sent);
    json::JSON internalapi_deleteMessage(SqliteConnection& conn, int64_t uid, const json::JSON& Sent);
    json::JSON internalapi_addMemberToChat(SqliteConnection& conn, int64_t uid, const json::JSON& Sent);
    json::JSON internalapi_removeMemberFromChat(SqliteConnection& conn, int64_t uid, const json::JSON& Sent);
    json::JSON internalapi_createChat(SqliteConnection& conn, int64_t uid, const json::JSON& Sent);
    json::JSON internalapi_leaveChat(SqliteConnection& conn, int64_t uid, const json::JSON& Sent);

    /**/
    void add_user(SqliteConnection& conn, const std::string& nickname, const std::string& name,
        const std::string& password, const std::string& bio, int64_t forced_id = -1);
    std::string admin_control_procedure(SqliteConnection& conn, const std::string& req, bool& termination);
}

#endif
