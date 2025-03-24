#ifndef ENGINE_ENGINE_NUMBER_9_MAINLOOP_H
#define ENGINE_ENGINE_NUMBER_9_MAINLOOP_H

#include "baza.h"
#include <functional>
#include <sys/time.h>
#include "os_utils.h"
#include "http_structures/client_request.h"
#include <stdint.h>
#include "socket_address.h"

namespace een9 {
    struct ConnectionInfo {
        SocketAddress server_name;
        SocketAddress client_name;
        /* 0 - http, 1 - 'een9::admin-control' protocol  */
        int type;
    };

    /* This structure is passed to guest function. It contains server info that might be or might be not used
     * by guest */
    struct EEN9_ServerTips {
        size_t server_load;
        size_t critical_load_1;
        timeval recommended_timeout;
    };

    struct SlaveTask {
        ConnectionInfo conn_info;
        UniqueFdWrapper fd;
        EEN9_ServerTips s_tips;
    };

    typedef int worker_id_t;

    /* guest_core function must not throw anything that is not derived from std::exception */
    typedef std::function<std::string(const SlaveTask&, const ClientRequest&, worker_id_t worker_id)> guest_core_t;
    /* same as gurst_core_t, but it used not for http, but for een9 specific "admin-cmd" protocol */
    typedef std::function<std::string(const SlaveTask&, const std::string&, worker_id_t)> guest_core_admin_control_t;

    struct ServersConfiguration {
        size_t critical_load_1 = 90;
        size_t critical_load_2 = 100;

        timeval request_timeout{20, 0};
    };

    struct MainloopParameters {
        /* On which addresses should I listed for incoming HTTP connections */
        std::vector<SocketAddress> client_regular_listened;
        /* On which addresses should I listen for incoming administrative commands */
        std::vector<SocketAddress> admin_control_listened;
        bool do_logging = true;
        size_t slave_number = 2;
        int mainloop_recheck_interval_us = 100;

        /* Takes parsed http request object. Should return fully-prepared http response */
        guest_core_t guest_core;
        /* Takes admin input. Returns only desired output message (without protocol header) */
        guest_core_admin_control_t guest_core_admin_control;

        ServersConfiguration s_conf;
    };

    void electric_boogaloo(const MainloopParameters& params, bool& termination_trigger);
}

#endif
