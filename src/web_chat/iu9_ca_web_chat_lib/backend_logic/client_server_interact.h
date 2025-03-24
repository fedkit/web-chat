#ifndef IU9_CA_WEB_CHAT_LIB_BACKEND_LOGIC_SERVER_DATA_INTERACT_PAGES_H
#define IU9_CA_WEB_CHAT_LIB_BACKEND_LOGIC_SERVER_DATA_INTERACT_PAGES_H

#include "server_data_interact.h"

#include "../sqlite3_wrapper.h"
#include <engine_engine_number_9/http_structures/client_request.h>
#include <engine_engine_number_9/http_structures/response_gen.h>
#include "../login_cookie.h"

#include <jsonincpp/jsonobj.h>
#include <new_york_transit_line/templater.h>
#include <memory>
#include "../localizator.h"

namespace iu9cawebchat {
    struct WorkerGuestData {
        /* Because templaters use libjsonincpp, they can't be READ by two thread simultaneously */
        std::unique_ptr<nytl::Templater> templater;
        std::unique_ptr<SqliteConnection> db;
        std::unique_ptr<Localizator> locales;
    };

    void initial_extraction_of_all_the_useful_info_from_cookies(
            SqliteConnection& conn, const een9::ClientRequest& req,
            std::vector<std::pair<std::string, std::string>>& ret_cookies,
            std::vector<LoginCookie>& ret_login_cookies,
            json::JSON& ret_userinfo,
            int64_t& ret_logged_in_user
        );

    std::string http_R200(const std::string& el_name, WorkerGuestData& wgd,
        const std::vector<const json::JSON*>& args);

    struct HtmlMsgBox {
        std::string class_;
        std::string text;
    };

    json::JSON jsonify_html_message_list(const std::vector<HtmlMsgBox>& messages);

    std::string page_E404(WorkerGuestData& wgd);

    /*  ========================== PAGES ================================== */

    std::string when_page_list_rooms(WorkerGuestData& wgd, const json::JSON& config_presentation,
                                     const een9::ClientRequest& req, const json::JSON& userinfo);

    std::string when_page_login(WorkerGuestData& wgd, const json::JSON& config_presentation,
                                const een9::ClientRequest& req, const std::vector<LoginCookie>& login_cookies, const json::JSON& userinfo);

    std::string when_page_chat(WorkerGuestData& wgd, const json::JSON& config_presentation,
                               const een9::ClientRequest& req, const json::JSON& userinfo);

    std::string when_page_user(WorkerGuestData& wgd, const json::JSON& config_presentation,
            const een9::ClientRequest& req, const std::vector<LoginCookie>& login_cookies, const json::JSON& userinfo);


    /* ========================  API  ============================== */
    std::string when_internalapi_chatpollevents(WorkerGuestData& wgd, const een9::ClientRequest& req, int64_t uid);

    std::string when_internalapi_chatlistpollevents(WorkerGuestData& wgd, const een9::ClientRequest& req, int64_t uid);

    std::string when_internalapi_getmessageneighbours(WorkerGuestData& wgd, const een9::ClientRequest& req, int64_t uid);

    std::string when_internalapi_sendmessage(WorkerGuestData &wgd, const een9::ClientRequest &req, int64_t uid);

    std::string when_internalapi_deletemessage(WorkerGuestData &wgd, const een9::ClientRequest &req, int64_t uid);

    std::string when_internalapi_addmembertochat(WorkerGuestData &wgd, const een9::ClientRequest &req, int64_t uid);

    std::string when_internalapi_removememberfromchat(WorkerGuestData &wgd, const een9::ClientRequest &req, int64_t uid);

    std::string when_internalapi_createchat(WorkerGuestData &wgd, const een9::ClientRequest &req, int64_t uid);

    std::string when_internalapi_leavechat(WorkerGuestData &wgd, const een9::ClientRequest &req, int64_t uid);
}

#endif
