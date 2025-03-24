#include "client_server_interact.h"
#include <engine_engine_number_9/form_data_structure/urlencoded_query.h>
#include <engine_engine_number_9/baza_throw.h>
#include <string.h>
#include "../str_fields.h"
#include "../login_cookie.h"

namespace iu9cawebchat {
    std::string get_user_bio(SqliteConnection& conn, int64_t userId) {
        een9_ASSERT(userId >= 0, "Are you crazy?");
        SqliteStatement sql_req(conn, "SELECT `bio` FROM `user` WHERE `id` = ?1", {{1, userId}}, {});
        fsql_text8_or_null bio_col;
        int status = sqlite_stmt_step(sql_req, {}, {{0, &bio_col}});
        if (status == SQLITE_ROW)
            return bio_col.value;
        een9_THROW("No such user");
    }

    static const char* pr_path = "/user/";

    bool is_page_of_certain_user(const std::string& path) {
        if (!een9::beginsWith(path, pr_path))
            return false;
        std::string r = path.substr(strlen(pr_path));
        return check_nickname(r);
    }

    std::string obtain_nickname_in_user_page_path(const std::string& path) {
        return path.substr(strlen(pr_path));
    }

    json::JSON user_row_to_userprofile_obj(SqliteConnection& conn, const RowUser_Content& alien) {
        return json::JSON(json::jdict{
            {"name", json::JSON(alien.name)},
            {"nickname", json::JSON(alien.nickname)},
            {"bio", json::JSON(get_user_bio(conn, alien.id))}
        });
    }

    std::string when_page_user(WorkerGuestData& wgd, const json::JSON& config_presentation,
                const een9::ClientRequest& req, const std::vector<LoginCookie>& login_cookies, const json::JSON& userinfo) {
        if (userinfo.isNull())
            return een9::form_http_server_response_303("/");
        SqliteConnection& conn = *wgd.db;
        if (!is_page_of_certain_user(req.uri_path))
            return page_E404(wgd);
        std::string alien_nickname = obtain_nickname_in_user_page_path(req.uri_path);
        RowUser_Content alien;
        try {
            alien = lookup_user_content_by_nickname(conn, alien_nickname);
        } catch (const std::exception& e) {
            return page_E404(wgd);
        }
        // todo: in libjsonincpp: fix '999999 problem'
        bool can_edit = false;
        int64_t myuid = -1;
        if (userinfo.isDictionary()) {
            myuid = userinfo["uid"].asInteger().get_int();
            can_edit = (alien.id == myuid && myuid >= 0);
        }
        if (req.method == "POST") {
            std::vector<std::pair<std::string, std::string>> response_hlines;
            try {
                if (!can_edit)
                    een9_THROW("Unauthorized access");
                std::vector<std::pair<std::string, std::string>> query = een9::split_html_query(req.body);
                // Profile update processing
                std::string bio;
                std::string name;
                std::string password;
                for (const std::pair<std::string, std::string>& p: query) {
                    if (p.first == "bio")
                        bio = p.second;
                    else if (p.first == "name")
                        name = p.second;
                    else if (p.first == "password")
                        password = p.second;
                }
                if (!bio.empty()) {
                    if (!is_orthodox_string(bio) || bio.size() > 100000)
                        een9_THROW("Incorrect `bio`");
                    sqlite_nooutput(conn,
                        "UPDATE `user` SET `bio` = ?1 WHERE `id` = ?2",
                        {{2, alien.id}}, {{1, bio}});
                }
                if (!name.empty()) {
                    if (!check_name(name))
                        een9_THROW("Incorrect `name`");
                    sqlite_nooutput(conn,
                        "UPDATE `user` SET `name` = ?1 WHERE `id` = ?2",
                        {{2, alien.id}}, {{1, name}});
                }
                if (!password.empty()) {
                    if (!check_strong_password(password))
                        een9_THROW("Incorrect `password`");
                    sqlite_nooutput(conn,
                        "UPDATE `user` SET `password` = ?1 WHERE `id` = ?2",
                        {{2, alien.id}}, {{1, password}});
                    if (alien.id == myuid) {
                        LoginCookie new_login_cookie = create_login_cookie(userinfo["nickname"].asString(), password);
                        add_set_cookie_headers_to_login(login_cookies, response_hlines, new_login_cookie);
                    }
                }
            } catch (const std::exception& e) {
                printf("Redirecting back to /user/... because of incorrect credentials\n");
                json::JSON msg_list = jsonify_html_message_list({{"",
                    config_presentation["edit-profile"]["incorrect-profile-data"].asString()}});
                json::JSON alien_userprofile = user_row_to_userprofile_obj(conn, alien);
                return http_R200("edit-profile", wgd, {&config_presentation, &userinfo, &alien_userprofile, &msg_list});
            }
            return een9::form_http_server_response_303_spec_head("/user/" + alien_nickname, response_hlines);
        }
        if (req.method == "GET") {
            json::JSON alien_userprofile = user_row_to_userprofile_obj(conn, alien);
            if (can_edit) {
                json::JSON empty_msg_list = jsonify_html_message_list({});
                return http_R200("edit-profile", wgd, {&config_presentation, &userinfo, &alien_userprofile, &empty_msg_list});
            }
            return http_R200("view-profile", wgd, {&config_presentation, &userinfo, &alien_userprofile});
        }
        een9_THROW("Bad method");
    }
}
