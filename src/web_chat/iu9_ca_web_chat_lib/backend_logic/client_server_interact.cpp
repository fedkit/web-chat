#include "client_server_interact.h"
#include <engine_engine_number_9/http_structures/cookies.h>
#include <functional>

namespace iu9cawebchat {
    void initial_extraction_of_all_the_useful_info_from_cookies(
        SqliteConnection& conn, const een9::ClientRequest& req,
        std::vector<std::pair<std::string, std::string>> &ret_cookies,
        std::vector<LoginCookie> &ret_login_cookies,
        json::JSON &ret_userinfo,
        int64_t& ret_logged_in_user
    ) {
        ret_cookies = een9::findAllClientCookies(req.headers);
        ret_login_cookies = select_login_cookies(ret_cookies);
        ret_logged_in_user = -1; /* Negative means that user is not logged in */
        if (!ret_login_cookies.empty()){
            size_t oldest_ind = select_oldest_login_cookie(ret_login_cookies);
            LoginCookie& tried = ret_login_cookies[oldest_ind];
            ret_logged_in_user = find_user_by_credentials(conn, tried.nickname, tried.password);
            if (ret_logged_in_user >= 0) {
                ret_userinfo["uid"] = json::JSON(ret_logged_in_user);
                ret_userinfo["nickname"] = json::JSON(tried.nickname);
                ret_userinfo["name"] = json::JSON(get_user_name(conn, ret_logged_in_user));
            }
        }
    }

    std::string http_R200(const std::string &el_name, WorkerGuestData &wgd,
        const std::vector<const json::JSON *> &args) {
        std::string page = wgd.templater->render(el_name, args);
        return een9::form_http_server_response_200("text/html", page);
    }

    json::JSON jsonify_html_message_list(const std::vector<HtmlMsgBox>& messages) {
        json::JSON jmessages(json::array);
        for (size_t i = 0; i < messages.size(); i++) {
            jmessages[i]["class"].asString() = messages[i].class_;
            jmessages[i]["text"].asString() = messages[i].text;
        }
        return jmessages;
    }

    std::string page_E404(WorkerGuestData &wgd) {
        return een9::form_http_server_response_404("text/html",
            wgd.templater->render("err-404", {}));
    }

    /* ========================= API =========================*/
    std::string when_internalapi(WorkerGuestData& wgd, const een9::ClientRequest& req, int64_t uid,
                                 const std::function<json::JSON(SqliteConnection&, int64_t, const json::JSON&)>& F) {
        const json::JSON& Sent = json::parse_str_flawless(req.body);
        std::string result = json::generate_str(F(*wgd.db, uid, Sent), json::print_pretty);
        return een9::form_http_server_response_200("text/json", result);
    }

    std::string when_internalapi_chatpollevents(WorkerGuestData& wgd, const een9::ClientRequest& req, int64_t uid) {
        return when_internalapi(wgd, req, uid, internalapi_chatPollEvents);
    }

    std::string when_internalapi_chatlistpollevents(WorkerGuestData &wgd, const een9::ClientRequest &req, int64_t uid) {
        return when_internalapi(wgd, req, uid, internalapi_chatListPollEvents);
    }

    std::string when_internalapi_getmessageneighbours(WorkerGuestData &wgd, const een9::ClientRequest &req, int64_t uid) {
        return when_internalapi(wgd, req, uid, internalapi_getMessageNeighbours);
    }

    std::string when_internalapi_sendmessage(WorkerGuestData &wgd, const een9::ClientRequest &req, int64_t uid) {
        return when_internalapi(wgd, req, uid, internalapi_sendMessage);
    }

    std::string when_internalapi_deletemessage(WorkerGuestData &wgd, const een9::ClientRequest &req, int64_t uid) {
        return when_internalapi(wgd, req, uid, internalapi_deleteMessage);
    }

    std::string when_internalapi_addmembertochat(WorkerGuestData &wgd, const een9::ClientRequest &req, int64_t uid) {
        return when_internalapi(wgd, req, uid, internalapi_addMemberToChat);
    }

    std::string when_internalapi_removememberfromchat(WorkerGuestData &wgd, const een9::ClientRequest &req, int64_t uid) {
        return when_internalapi(wgd, req, uid, internalapi_removeMemberFromChat);
    }

    std::string when_internalapi_createchat(WorkerGuestData &wgd, const een9::ClientRequest &req, int64_t uid) {
        return when_internalapi(wgd, req, uid, internalapi_createChat);
    }

    std::string when_internalapi_leavechat(WorkerGuestData &wgd, const een9::ClientRequest &req, int64_t uid) {
        return when_internalapi(wgd, req, uid, internalapi_leaveChat);
    }
}
