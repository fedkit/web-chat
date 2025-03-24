#include "server_data_interact.h"
#include <engine_engine_number_9/baza_throw.h>
#include "../str_fields.h"
#include <string.h>

namespace iu9cawebchat {
    /* nagative `forced_id` means id isn't forced  */
    void add_user(SqliteConnection& conn, const std::string& nickname, const std::string& name,
        const std::string& password, const std::string& bio, int64_t forced_id) {
        if (!check_nickname(nickname))
            een9_THROW("Bad user nickname " + nickname + ". Can't reg");
        if (!check_name(name))
            een9_THROW("Bad user name " + name + ". Can't reg");
        if (!check_strong_password(password))
            een9_THROW("Bad user password. Can't reg");
        if (!is_orthodox_string(bio))
            een9_THROW("Bad user bio. Can't reg");
        if (is_nickname_taken(conn, nickname))
            een9_THROW("Nickname taken already. Can't reg");
        reserve_nickname(conn, nickname);
        SqliteStatement req(conn,
            "INSERT INTO `user` (`id`, `nickname`, `name`, `chatList_HistoryId`, `password`, `bio`) "
            "VALUES (?1, ?2, ?3, 0, ?4, ?5)", {}, {{2, nickname}, {3, name}, {4, password}, {5, bio}});
        if (forced_id >= 0)
            sqlite_stmt_bind_int64(req, 1, forced_id);
        int must_be_done = sqlite_stmt_step(req, {}, {});
        if (must_be_done != SQLITE_DONE)
            een9_THROW("sqlite error");
    }

    std::string admin_control_procedure(SqliteConnection& conn, const std::string& req, bool& termination) {
        size_t nid = 0;
        auto read_thing = [&]() -> std::string {
            while (nid < req.size() && isSPACE(req[nid]))
                nid++;
            std::string result;
            bool esc = false;
            while (nid < req.size() && (esc || !isSPACE(req[nid]))) {
                if (esc) {
                    result += req[nid];
                    esc = false;
                } else if (req[nid] == '\\') {
                    esc = true;
                } else {
                    result += req[nid];
                }
                nid++;
            }
            return result;
        };

        std::string cmd = read_thing();
        if (cmd == "hello") {
            return ":0 omg! hiii!! Hewwou :3 !!!!\n";
        }
        if (cmd == "8") {
            termination = true;
            return "Bye\n";
        }
        const char* adduser_pref = "adduser";
        if (cmd == "updaterootpw") {

            std::string new_password = read_thing();
            if (!check_strong_password(new_password))
                return "Bad password. Can't update";
            sqlite_nooutput(conn,
                "UPDATE `user` SET `password` = ?1 WHERE `id` = 0", {}, {{1, new_password}});
            return "Successul update\n";
        }
        /* adduser <nickname> <name> <password> <bio> */
        if (cmd == "adduser") {
            std::string nickname = read_thing();
            std::string name = read_thing();
            std::string password = read_thing();
            std::string bio = read_thing();
            add_user(conn, nickname, name, password, bio);
            return "User " + nickname + " successfully registered";
        }
        return "Incorrect command\n";
    }
}
