#include <iu9_ca_web_chat_lib/backend_logic/server_data_interact.h>
#include <iu9_ca_web_chat_lib/sqlite3_wrapper.h>
#include <jsonincpp/string_representation.h>
#include <assert.h>

using namespace iu9cawebchat;

int main() {
    SqliteConnection conn("./iu9-ca-web-chat.db");
    for (size_t i = 0; i < 100; i++) {
        int64_t uid = find_user_by_credentials(conn, "root", "12345678");
        assert(uid == 0);
    }
}