#include "running_mainloop.h"

#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <assert.h>
#include <map>
#include <queue>
#include <utility>
#include "thread_synchronization.h"
#include "os_utils.h"
#include "http_structures/client_request_parse.h"
#include "http_structures/response_gen.h"
#include "baza_inter.h"
#include "admin_control.h"

namespace een9 {
    struct QElementHttpConnections {
        SlaveTask task;
        QElementHttpConnections* nxt = NULL;

        explicit QElementHttpConnections(SlaveTask task): task(std::move(task)) {}
    };

    struct WorkersTaskQueue {
        QElementHttpConnections* first = NULL;
        QElementHttpConnections** afterLastPtr;
        size_t sz = 0;

        WorkersTaskQueue() {
            afterLastPtr = &first;
        }

        bool empty() const {
            return sz == 0;
        }

        size_t size() const {
            return sz;
        }

        void push_back(SlaveTask task) {
            /* CLion says. Allocated memory is leaking. YOUR MOTHER IS LEAKING YOU FOOL!! MY CODE IS FINE!! */
            QElementHttpConnections* el = new QElementHttpConnections(std::move(task));
            /* Exception does not leave queue in incorrect state */
            *afterLastPtr = el;
            afterLastPtr = &(el->nxt);
            sz++;
        }

        void pop_first(SlaveTask& ret_task) {
            assert(!empty());
            ret_task = std::move(first->task);
            if (sz == 1) {
                delete first;
                first = NULL;
                afterLastPtr = &first;
                sz = 0;
            } else {
                /* Before I popped the first, this element was second, but now it took place of the first */
                QElementHttpConnections* old_deut = first->nxt;
                delete first;
                first = old_deut;
                sz--;
            }
        }

        ~WorkersTaskQueue() {
            QElementHttpConnections* cur = first;
            while (cur) {
                QElementHttpConnections* nxt = cur->nxt;
                delete cur;
                cur = nxt;
            }
        }
    };

    struct WorkersEnvCommon {
        /* This alarm notifies about new tasks and termination signal. Because we are polite people, we don't cancel threads */
        CondVarBedObj corvee_bed;
        WorkersTaskQueue queue;
        bool& termination;
        guest_core_t guest_core;
        guest_core_admin_control_t guest_core_admin_control;

        /* Parser programs */
        ClientRequestParser_CommonPrograms parser_programs;

        WorkersEnvCommon(bool& term, const MainloopParameters& params): termination(term),
        guest_core(params.guest_core), guest_core_admin_control(params.guest_core_admin_control){}
    };

    struct WorkersEnv {
        WorkersEnvCommon& wtec;
        worker_id_t id;
        ClientRequestParser_WorkerBuffers personal_parser_buffer;

        explicit WorkersEnv(WorkersEnvCommon& wtec, worker_id_t id): wtec(wtec), id(id), personal_parser_buffer(wtec.parser_programs){}
    };

    // todo: add timeout for multiple bytes, add more settings
    ClientRequest process_http_connection_input(int fd, const EEN9_ServerTips& s_tips, WorkersEnv& wte) {
        ClientRequest res;
        ClientHttpRequestParser_Ctx parser(res, wte.personal_parser_buffer, wte.wtec.parser_programs);
        int ret;
        char buf[2048];
        assert(parser.status == 0);
        while ((ret = (int)recv(fd, buf, 2048, 0)) > 0) {
            for (size_t i = 0; i < ret; i++) {
                if (parser.feedCharacter(buf[i]) != 0)
                    break;
            }
            if (parser.status != 0)
                break;
        }
        ASSERT(parser.status == 1, "Incorrect request"); // todo: do the same thing everywhere else
        ASSERT_on_iret(ret, "recv");
        return res;
    }

    void process_connection_output(int fd, const std::string& server_response) {
        size_t N = server_response.size(), i = 0;
        while (i < N) {
            /* MSG_NOSIGNAL set to prevent SIGPIPE */
            int written = (int)send(fd, &server_response[i], std::min(2048lu, N - i), MSG_NOSIGNAL);
            ASSERT_on_iret(written, "sending");
            ASSERT_pl(written > 0);
            i += written;
        }
        printf("Log: worker: succesfully asnwered with response\n");
    }

    std::string process_admin_control_connection_input(int fd, const EEN9_ServerTips& s_tips, WorkersEnv& wte) {
        AdminControlRequestRCtx pctx;
        int ret;
        char buf[2048];
        assert(pctx.status == 0);
        while ((ret = (int)recv(fd, buf, 2048, 0)) > 0) {
            for (size_t i = 0; i < ret; i++) {
                if (pctx.feedCharacter(buf[i]) != 0)
                    break;
            }
            if (pctx.status != 0)
                break;
        }
        ASSERT(pctx.status == 1, "Incorrect request");
        ASSERT_on_iret(ret, "recv");
        return pctx.body;
    }

    void process_connection(const SlaveTask& task, WorkersEnv& wte) {
        if (task.conn_info.type == 0) {
            printf("%d::Got http reuest\n", wte.id);
            ClientRequest client_request = process_http_connection_input(task.fd(), task.s_tips, wte);
            printf("%d::Http request has been read\n", wte.id);
            std::string server_response = wte.wtec.guest_core(task, client_request, wte.id);
            process_connection_output(task.fd(), server_response);
            printf("%d::Http response has been sent\n", wte.id);
        } else if (task.conn_info.type == 1) {
            printf("%d::Got admin-cmd request\n", wte.id);
            std::string admin_request = process_admin_control_connection_input(task.fd(), task.s_tips, wte);
            printf("%d::Admin-cmd request has been read\n", wte.id);
            std::string server_response_content = wte.wtec.guest_core_admin_control(task, admin_request, wte.id);
            std::string server_response = generate_admin_control_response(server_response_content);
            process_connection_output(task.fd(), server_response);
            printf("%d::Admin-cmd response has been sent\n", wte.id);
        }
    }

    void* worker_func(void* wte_ptr) {
        WorkersEnv& wte = *((WorkersEnv*)wte_ptr);
        WorkersEnvCommon& wtec = wte.wtec;
        printf("Worker started\n");
        while (true) {
            try {
                MutexLockGuard cb_lg(wtec.corvee_bed, __func__);
                woke:
                if (wtec.termination)
                    break;
                if (wtec.queue.empty()) {
                    wtec.corvee_bed.sleep(__func__);
                    goto woke;
                }
                SlaveTask task;
                wtec.queue.pop_first(task);
                process_connection(task, wte);
            } catch (const std::exception& e) {
                printf("Client request procession failure in worker\n");
                printf("%s\n", e.what());
                /* Under mysterious some circumstances, in this place destructor of string in SystemError causes segfault. I can't fix that */
            }
        }
        printf("Worker finished\n");
        return NULL;
    }

    void electric_boogaloo(const MainloopParameters& params, bool& termination_trigger) {
        WorkersEnvCommon wtec(termination_trigger, params);
        ASSERT(params.slave_number > 0, "No workers spawned");
        size_t CRL_Nip = params.client_regular_listened.size();
        ASSERT(CRL_Nip > 0, "No open listeting addresses (http)");
        size_t ACL_Nip = params.admin_control_listened.size();
        size_t Nip = CRL_Nip + ACL_Nip;

        std::vector<pthread_t> workers(params.slave_number);
        std::vector<uptr<WorkersEnv>> wtes(params.slave_number);
        for (size_t i = 0; i < params.slave_number; i++) {
            wtes[i] = std::make_unique<WorkersEnv>(wtec, (worker_id_t)i);
        }
        for (size_t i = 0; i < params.slave_number; i++) {
            pthread_create(&workers[i], NULL, worker_func, wtes[i].get());
        }

        // todo: right now this try block protects threads. So I need to put pthreads in some kind of guarding object
        try {
            int ret;
            struct Ear {
                /* A copy from params */
                SocketAddress my_addr;
                UniqueFdWrapper listening_sock;
                /* type 0 is for http protocol
                 * type 1 is for admin-cmd protocol */
                int type;
            };
            std::vector<Ear> ears(Nip);
            for (size_t i = 0; i < CRL_Nip; i++) {
                ears[i].my_addr = params.client_regular_listened[i];
                ears[i].type = 0;
            }
            for (size_t i = 0; i < ACL_Nip; i++) {
                ears[i + CRL_Nip].my_addr = params.admin_control_listened[i];
                ears[i + CRL_Nip].type = 1;
            }
            for (size_t i = 0; i < Nip; i++) {
                int listening_socket_fd = socket(ears[i].my_addr.v.gen.sa_family, SOCK_STREAM, 0);
                ASSERT_on_iret(listening_socket_fd, "'Listening socket' creation");
                UniqueFdWrapper listening_socket(listening_socket_fd);
                int reuseaddr_nozero_option_value = 1;
                ret = setsockopt(listening_socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_nozero_option_value, sizeof(int));
                ASSERT_on_iret(ret, "Can't set SO_REUSEADDR");
                bind_to_socket_address(listening_socket_fd, ears[i].my_addr);
                ret = listen(listening_socket(), 128);
                ASSERT_on_iret(ret, "listen() listening for connections");
                ears[i].listening_sock = std::move(listening_socket);
            }
            std::vector<pollfd> pollfds(Nip);
            for (size_t i = 0; i < Nip; i++) {
                pollfds[i].fd = ears[i].listening_sock();
                pollfds[i].events = POLLRDNORM;
            }
            ASSERT(params.mainloop_recheck_interval_us > 0, "Incorrect poll timeout");
            while (true) {
                if (wtec.termination)
                    break;
                for (size_t i = 0; i < Nip; i++) {
                    pollfds[i].revents = 0;
                }
                errno = 0;
                ret = poll(pollfds.data(), Nip, params.mainloop_recheck_interval_us);
                if (ret != 0 && errno != 0) {
                    printf("poll() error :> %s\n", een9::prettyprint_errno("").c_str());
                    continue;
                }
                for (size_t i = 0; i < Nip; i++) {
                    if ((pollfds[i].revents & POLLRDNORM)) {
                        try {
                            int session_sock = accept(pollfds[i].fd, NULL, NULL);
                            ASSERT_on_iret(session_sock, "Failed to accept incoming connection");
                            UniqueFdWrapper session_sock_fdw(session_sock);
                            configure_socket_rcvsndtimeo(session_sock_fdw(), params.s_conf.request_timeout);
                            SocketAddress peer_addr;
                            get_peer_name_as_socket_address(session_sock, peer_addr);
                            { MutexLockGuard lg2(wtec.corvee_bed, "poller adds connection");
                                SlaveTask task{
                                    ConnectionInfo{ears[i].my_addr, peer_addr, ears[i].type},
                                    std::move(session_sock_fdw),
                                    EEN9_ServerTips{wtec.queue.size(),
                                        params.s_conf.critical_load_1, params.s_conf.request_timeout}
                                };
                                if (wtec.queue.size() < params.s_conf.critical_load_2)
                                    wtec.queue.push_back(std::move(task));
                            }
                            wtec.corvee_bed.din_don();
                        } catch (const std::exception& e) {
                            printf("Error aceepting connection\n");
                            printf("%s\n", e.what());
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            printf("System failure 2\n");
            printf("%s\n", e.what());
            /* There is no need to tiptoe around this multi-access field. It is write-once-and-for-good-kind  */
            wtec.termination = true;
            wtec.corvee_bed.wake_them_all();
        }
        wtec.termination = true;
        wtec.corvee_bed.wake_them_all();
        for (size_t i = 0; i < params.slave_number; i++) {
            pthread_join(workers[i], NULL);
        }
    }
}
