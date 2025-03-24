#include "regexis024_build_system.h"
#include <utility>

std::string make_uppercase(const std::string &source) {
    std::string result(source);
    for (size_t i = 0; i < source.size(); i++) {
        char ch = source[i];
        if ('a' <= ch && ch <= 'z')
            result[i] = (char)(ch - 'a' + 'A');
    }
    return result;
}

ExternalLibraryTarget formExternalLibraryTargetWithNonNativeName(const std::string& name) {
    std::string ev_name = "BSCRIPT_DEP_" + make_uppercase(name);
    const char* ev = getenv(ev_name.c_str());
    ASSERT(ev, "No environmaent variable " + ev_name);
    return {name, parse_passed_forward_str(ev)};
}

struct CAWebChat {
    /* Building runlevel */
    BuildUnitsArray runlevel_1;
    /* Installation runlevel */
    BuildUnitsArray runlevel_2;

    std::string build_type;
    bool make_tests = false;

    std::vector<std::string> warning_flags = {"-Wall", "-Wno-unused-variable", "-Werror=return-type","-pedantic",
        "-Wno-unused-but-set-variable", "-Wno-reorder"};
    std::vector<std::string> version_flags = {"--std", "c++17", "-D", "_POSIX_C_SOURCE=200809L"};
    std::vector<std::string> debug_defines_release = {"_GLIBCXX_DEBUG"};
    std::vector<std::string> debug_defines_debug = {"_GLIBCXX_DEBUG", "DEBUG_ALLOW_LOUD"};
    std::vector<std::string> opt_flags_release = {"-g", "-O2"};
    std::vector<std::string> opt_flags_debug = {"-g", "-ggdb", "-O0"};

    std::vector<std::string> getSomeRadFlags() const {
        std::vector<std::string> my_flag_collection;
        gxx_add_cli_options(my_flag_collection, warning_flags);
        gxx_add_cli_options(my_flag_collection, version_flags);
        if (build_type == "release") {
            gxx_add_cli_defines(my_flag_collection, debug_defines_release);
            gxx_add_cli_options(my_flag_collection, opt_flags_release);
        } else if (build_type == "debug") {
            gxx_add_cli_defines(my_flag_collection, debug_defines_debug);
            gxx_add_cli_options(my_flag_collection, opt_flags_debug);
        }
        return my_flag_collection;
    }

    explicit CAWebChat(const NormalCBuildSystemCommandMeaning& cmd){
        const char* BSCRIPT_TYPE = getenv("BSCRIPT_TYPE");
        const char* BSCRIPT_TESTS = getenv("BSCRIPT_TESTS");
        build_type = BSCRIPT_TYPE ? BSCRIPT_TYPE : "release";
        make_tests = (bool)BSCRIPT_TESTS;
        ASSERT(build_type == "release" || build_type == "debug", "Unknown build type");

        std::vector<ExternalLibraryTarget> ext_targets = {
            formExternalLibraryTargetWithNonNativeName("libjsonincpp"),
            formExternalLibraryTargetWithNonNativeName("sqlite3"),
            formExternalLibraryTargetWithNonNativeName("libregexis024"),
        };

        std::vector<CTarget> my_targets;

        { CTarget T{"engine_engine_number_9", "shared_library"};
            T.additional_compilation_flags = getSomeRadFlags();
            T.proj_deps = {};
            T.external_deps = {
                CTargetDependenceOnExternalLibrary{"libjsonincpp", {true, true}},
                CTargetDependenceOnExternalLibrary{"libregexis024", {true, true}}
            };
            T.units = {
                "baza.cpp",
                "thread_synchronization.cpp",
                "os_utils.cpp",
                "http_structures/client_request_parse.cpp",
                "http_structures/response_gen.cpp",
                "http_structures/cookies.cpp",
                "http_structures/accept_language.cpp",
                "connecting_assets/static_asset_manager.cpp",
                "running_mainloop.cpp",
                "form_data_structure/urlencoded_query.cpp",
                "socket_address.cpp",
                "admin_control.cpp",
            };
            for (std::string& u: T.units)
                u = "http_server/engine_engine_number_9/" + u;
            T.include_pr = "http_server";
            T.include_ir = "";
            T.exported_headers = {
                "baza.h",
                "baza_throw.h",
                "thread_synchronization.h",
                "os_utils.h",
                "connecting_assets/static_asset_manager.h",
                "http_structures/client_request.h",
                "http_structures/cookies.h",
                "http_structures/response_gen.h",
                "http_structures/accept_language.h",
                "running_mainloop.h",
                "form_data_structure/urlencoded_query.h",
                "socket_address.h",
                "admin_control.h",
            };
            for (std::string& u: T.exported_headers)
                u = "engine_engine_number_9/" + u;
            T.installation_dir = "een9";
            my_targets.push_back(T);
        }
        { CTarget T{"new_york_transit_line", "shared_library"};
            T.additional_compilation_flags = getSomeRadFlags();
            T.external_deps = {
                CTargetDependenceOnExternalLibrary{"libjsonincpp", {true, true}},
            };
            T.units = {
                "alotalot.cpp",
                "html_case.cpp",
                "parser.cpp",
                "rendering.cpp",
                "templater.cpp",
            };
            for (std::string& u: T.units)
                u = "http_server/new_york_transit_line/" + u;
            T.include_pr = "http_server";
            T.exported_headers = {
                "templater.h",
                "html_case.h",
            };
            for (std::string& u: T.exported_headers)
                u = "new_york_transit_line/" + u;
            T.installation_dir = "nytl";
            my_targets.push_back(T);
        }
        { CTarget T{"iu9_ca_web_chat_lib", "shared_library"};
            T.additional_compilation_flags = getSomeRadFlags();
            T.proj_deps = {
                CTargetDependenceOnProjectsLibrary{"engine_engine_number_9", {true, true}},
                CTargetDependenceOnProjectsLibrary{"new_york_transit_line", {true, true}},
            };
            T.external_deps = {
                CTargetDependenceOnExternalLibrary{"sqlite3", {true, true}}
            };
            T.units = {
                "localizator.cpp",
                "initialize.cpp",
                "run.cpp",
                "str_fields.cpp",
                "find_db.cpp",
                "sqlite3_wrapper.cpp",
                "login_cookie.cpp",
                "backend_logic/server_data_interact.cpp",
                "backend_logic/client_server_interact.cpp",

                "backend_logic/when_login.cpp",
                "backend_logic/when_list_rooms.cpp",
                "backend_logic/when_chat.cpp",
                "backend_logic/when_user.cpp",
                "backend_logic/polling.cpp",
                "backend_logic/api_sendmessage.cpp",
                "backend_logic/api_deletemessage.cpp",
                "backend_logic/api_addmembertochat.cpp",
                "backend_logic/api_removememberfromchat.cpp",
                "backend_logic/api_createchat.cpp",
                "backend_logic/api_leavechat.cpp",
                "backend_logic/admin_control_procedure.cpp",
            };
            for (std::string& u: T.units)
                u = "web_chat/iu9_ca_web_chat_lib/" + u;
            T.exported_headers = {
                "actions.h"
            };
            for (std::string& u: T.exported_headers)
                u = "iu9_ca_web_chat_lib/" + u;
            T.include_pr = "web_chat";
            T.installation_dir = "iu9cawebchat";
            my_targets.push_back(T);
        }
        { CTarget T{"iu9-ca-web-chat-service", "executable"};
            T.additional_compilation_flags = getSomeRadFlags();
            T.proj_deps = {
                CTargetDependenceOnProjectsLibrary{"iu9_ca_web_chat_lib"},
            };
            T.units = {"service.cpp"};
            for (std::string& u: T.units)
                u = "web_chat/iu9_ca_web_chat_service/" + u;
            T.include_pr = "web_chat";
            my_targets.push_back(T);
        }
        { CTarget T{"iu9-ca-web-chat-admin-cli", "executable"};
            T.additional_compilation_flags = getSomeRadFlags();
            T.proj_deps = {
                CTargetDependenceOnProjectsLibrary{"engine_engine_number_9"},
            };
            T.units = {
                "cli.cpp",  // Main file
            };
            for (std::string& u: T.units)
                u = "web_chat/iu9_ca_web_chat_admin_cli/" + u;
            T.include_pr = "web_chat";
            my_targets.push_back(T);
        }
        regular_ctargets_to_2bus_conversion(ext_targets, my_targets, runlevel_1, runlevel_2,
            cmd.project_root, cmd.installation_root);
    }
};

int main(int argc, char** argv) {
    try {
        ASSERT_pl(argc > 0);
        std::vector<std::string> args(argc - 1);
        for (int i = 0; i + 1 < argc; i++) {
            args[i] = argv[i + 1];
        }
        NormalCBuildSystemCommandMeaning cmd;
        regular_bs_cli_cmd_interpret(args, cmd);
        CAWebChat bs(cmd);
        if (cmd.need_to_build)
            complete_tasks_of_build_units(bs.runlevel_1);
        umask(~0755);
        if (cmd.need_to_install)
            complete_tasks_of_build_units(bs.runlevel_2);
    } catch (const buildSystemFailure& e) {
        printf("Build system failure\n""%s\n", e.toString().c_str());
    }
}
