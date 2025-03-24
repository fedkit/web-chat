#include <iu9_ca_web_chat_lib/backend_logic/server_data_interact.h>
#include <iu9_ca_web_chat_lib/sqlite3_wrapper.h>
#include <jsonincpp/string_representation.h>

using namespace iu9cawebchat;

void test(SqliteConnection& conn, int64_t uid){
    json::JSON Recv = internalapi_getChatList(conn, uid);
    printf("%s\n", json::generate_str(Recv, json::print_pretty).c_str());
}

void test_polling(SqliteConnection& conn, int64_t uid, int64_t LocalHistoryId) {
    json::JSON Sent;
    Sent["scope"][0]["type"] = json::JSON("chatlist");
    Sent["scope"][0]["LocalHistoryId"] = json::JSON(LocalHistoryId);

    json::JSON Recv = internalapi_pollEvents(conn, uid, Sent);
    printf("%s\n", json::generate_str(Recv, json::print_pretty).c_str());
}

int main() {
    SqliteConnection conn("./iu9-ca-web-chat.db");
    // test(conn, 0);
    // test(conn, 1);
    // test(conn, 2);
    // printf("\n\n ===== Now testing polling of events ===== \n\n");
    test_polling(conn, 1, 0);
    test_polling(conn, 1, 1);
    test_polling(conn, 1, 2);
    return 0;
}
