#include "actions.h"

#include <engine_engine_number_9/baza_throw.h>
#include <engine_engine_number_9/os_utils.h>
#include <engine_engine_number_9/connecting_assets/static_asset_manager.h>
#include "find_db.h"
#include <engine_engine_number_9/running_mainloop.h>
#include <engine_engine_number_9/http_structures/accept_language.h>
#include <signal.h>
#include "str_fields.h"
#include "backend_logic/client_server_interact.h"

namespace iu9cawebchat {
    bool termination = false;

    void sigterm_action(int) {
        termination = true;
    }

    struct ONE_SQLITE_TRANSACTION_GUARD {
        SqliteConnection& conn;
        bool rollback = false;

        explicit ONE_SQLITE_TRANSACTION_GUARD(SqliteConnection& conn_) : conn(conn_) {
            sqlite_nooutput(conn, "BEGIN", {}, {});
        }

        ~ONE_SQLITE_TRANSACTION_GUARD() {
            if (rollback)
                sqlite_nooutput(conn, "ROLLBACK", {}, {});
            else
                sqlite_nooutput(conn, "END", {}, {});
        }
    };

    LocalizatorSettings make_localizator_settings(const std::string& assets_dir, const json::JSON& config) {
        std::vector<std::string> whitelist;
        for (const json::JSON& entry: config["lang"]["whitelist"].asArray())
            whitelist.push_back(languageRangeSimpler(entry.asString()));
        std::vector<std::string> force_order;
        for (const json::JSON& entry: config["lang"]["force-order"].asArray())
            force_order.push_back(languageRangeSimpler(entry.asString()));
        return LocalizatorSettings{assets_dir + "/lang", whitelist, force_order};
    }

    void run_website(const json::JSON& config) {
        een9_ASSERT(config["assets"].isString(), "config[\"assets\"] is not string");
        const std::string& assets_dir = config["assets"].asString();
        een9_ASSERT(een9::isDirectory(assets_dir), "\"" + assets_dir + "\" is not a directory");

        LocalizatorSettings localizator_settings = make_localizator_settings(assets_dir, config);

        een9::StaticAssetManagerSlaveModule samI;
        samI.update({
            een9::StaticAssetManagerRule{assets_dir + "/css", "/assets/css", {{".css", "text/css"}} },
            een9::StaticAssetManagerRule{assets_dir + "/js", "/assets/js", {{".js", "text/javascript"}} },
            een9::StaticAssetManagerRule{assets_dir + "/gif", "/assets/gif", {{".gif", "image/gif"}} },
            een9::StaticAssetManagerRule{assets_dir + "/img", "/assets/img", {
                {".jpg", "image/jpg"}, {".png", "image/png"}, {".svg", "image/svg+xml"}
            } },
        });

        int64_t slave_number = config["server"]["workers"].asInteger().get_int();
        een9_ASSERT(slave_number > 0 && slave_number <= 200, "E");

        std::string sqlite_db_path;
        int ret = find_db_sqlite_file_path(config, sqlite_db_path);
        een9_ASSERT(ret == 0, "Can't find database file");

        std::vector<WorkerGuestData> worker_guest_data(slave_number);
        for (int i = 0; i < slave_number; i++) {
            worker_guest_data[i].templater = std::make_unique<nytl::Templater>(
                nytl::TemplaterSettings{nytl::TemplaterDetourRules{assets_dir + "/HypertextPages"}});
            worker_guest_data[i].templater->update();
            worker_guest_data[i].db = std::make_unique<SqliteConnection>(sqlite_db_path);
            worker_guest_data[i].locales = std::make_unique<Localizator>(localizator_settings);
        }

        een9::MainloopParameters params;
        params.guest_core = [&samI, &worker_guest_data]
        (const een9::SlaveTask& task, const een9::ClientRequest& req, een9::worker_id_t worker_id) -> std::string {
            een9_ASSERT_pl(0 <= worker_id && worker_id < worker_guest_data.size());
            WorkerGuestData& wgd = worker_guest_data[worker_id];
            een9::StaticAsset sa;
            ONE_SQLITE_TRANSACTION_GUARD conn_guard(*wgd.db);
            std::string AcceptLanguage;
            for (const std::pair<std::string, std::string>& p: req.headers) {
                if (p.first == "Accept-Language")
                    AcceptLanguage = p.second;
            }
            std::vector<std::string> AcceptLanguageB = een9::parse_header_Accept_Language(AcceptLanguage);
            const LanguageFile& locale = wgd.locales->get_right_locale(AcceptLanguageB);
            const json::JSON& pres = locale.content;
            try {
                std::vector<std::pair<std::string, std::string>> cookies;
                std::vector<LoginCookie> login_cookies;
                json::JSON userinfo;
                int64_t logged_in_user = -1;
                initial_extraction_of_all_the_useful_info_from_cookies(*wgd.db, req, cookies, login_cookies, userinfo, logged_in_user);

                if (req.uri_path == "/" || req.uri_path == "/list-rooms") {
                    return when_page_list_rooms(wgd, pres, req, userinfo);
                }
                if (req.uri_path == "/login") {
                    return when_page_login(wgd, pres, req, login_cookies, userinfo);
                }
                // todo: split
                if (een9::beginsWith(req.uri_path, "/chat/") || een9::beginsWith(req.uri_path, "/chat-members/")) {
                    return when_page_chat(wgd, pres, req, userinfo);
                }
                if (een9::beginsWith(req.uri_path, "/user/")) {
                    return when_page_user(wgd, pres, req, login_cookies, userinfo);
                }
                if (req.uri_path == "/api/chatPollEvents") {
                    return when_internalapi_chatpollevents(wgd, req, logged_in_user);
                }
                if (req.uri_path == "/api/chatListPollEvents") {
                    return  when_internalapi_chatlistpollevents(wgd, req, logged_in_user);
                }
                if (req.uri_path == "/api/getMessageNeighbours") {
                    return when_internalapi_getmessageneighbours(wgd, req, logged_in_user);
                }
                if (req.uri_path == "/api/sendMessage") {
                    return when_internalapi_sendmessage(wgd, req, logged_in_user);
                }
                if (req.uri_path == "/api/deleteMessage") {
                    return when_internalapi_deletemessage(wgd, req, logged_in_user);
                }
                if (req.uri_path == "/api/addMemberToChat") {
                    return when_internalapi_addmembertochat(wgd, req, logged_in_user);
                }
                if (req.uri_path == "/api/removeMemberFromChat") {
                    return when_internalapi_removememberfromchat(wgd, req, logged_in_user);
                }
                if (req.uri_path == "/api/createChat") {
                    return when_internalapi_createchat(wgd, req, logged_in_user);
                }
                if (req.uri_path == "/api/leaveChat") {
                    return when_internalapi_leavechat(wgd, req, logged_in_user);
                }
            } catch (const std::exception& e) {
                conn_guard.rollback = true;
                throw;
            }
            /* Trying to interpret request as asset lookup */
            int rets = samI.get_asset(req.uri_path, sa);
            if (rets >= 0) {
                return een9::form_http_server_response_200(sa.type, sa.content);
            }
            return een9::form_http_server_response_404("text/html", "<h1> Not found! </h1>");
        };

        params.guest_core_admin_control = [&worker_guest_data]
        (const een9::SlaveTask& task, const std::string& req, een9::worker_id_t worker_id) -> std::string {
            een9_ASSERT_pl(0 <= worker_id && worker_id < worker_guest_data.size());
            WorkerGuestData& wgd = worker_guest_data[worker_id];
            ONE_SQLITE_TRANSACTION_GUARD conn_guad(*wgd.db);
            try {
                return admin_control_procedure(*wgd.db, req, termination);
            } catch (std::exception& e) {
                conn_guad.rollback = true;
                return std::string("Server error\n") + e.what();
            }
        };

        params.slave_number = slave_number;

        een9::SocketAddressParser sap;
        auto translate_addr_list_conf = [&sap](std::vector<een9::SocketAddress>& dest, const std::vector<json::JSON>& source) {
            size_t N = source.size();
            dest.resize(N);
            for (size_t i = 0; i < N; i++) {
                int ret = een9::parse_socket_address(source[i].asString(), dest[i], sap);
                een9_ASSERT(ret == 0, "Incorrect ear address: " + source[i].asString());
            }
        };
        translate_addr_list_conf(params.client_regular_listened, config["server"]["http-listen"].asArray());
        translate_addr_list_conf(params.admin_control_listened, config["server"]["admin-command-listen"].asArray());

        signal(SIGINT, sigterm_action);
        signal(SIGTERM, sigterm_action);

        een9::electric_boogaloo(params, termination);
    }
}
