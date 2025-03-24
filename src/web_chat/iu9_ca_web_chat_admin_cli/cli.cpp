#include <engine_engine_number_9/admin_control.h>
#include <engine_engine_number_9/socket_address.h>
#include <engine_engine_number_9/baza_throw.h>
#include <assert.h>
#include <engine_engine_number_9/os_utils.h>

/* This so called 'een9::admin-control' protocol is very simple:
 * Admin sends request to server:
 * <Magic constant string> <8 byte field: size of body> <body (string of any characters)>
 * Server reads it to the end and sents response to admin:
 * <Magic constant string> <8 byte field: size of body> <body (string of any characters)>
 * More can be found in src/http_server/engine_engine_number_9/admin_control.cpp
 */

void send_request_msg(int fd, const std::string& request_msg) {
    std::string str = een9::generate_admin_control_request(request_msg);
    size_t N = str.size(), i = 0;
    while (i < N) {
        int written = (int)send(fd, &str[i], std::min(2048lu, N - i), MSG_NOSIGNAL);
        een9_ASSERT_on_iret(written, "sending");
        een9_ASSERT_pl(written > 0);
        i += written;
    }
}

std::string receive_response_msg(int fd) {
    een9::AdminControlResponseRCtx pctx;
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
    een9_ASSERT(pctx.status == 1, "Received incorrect response");
    een9_ASSERT_on_iret(ret, "recv");
    return pctx.body;
}

void usage(char* za) {
    printf("%s <address of servers admin cmd listener> <message> [<other parts of message> ...]\n", za);
    exit(1);
}

void funny_log_print(const char* str) {
    printf("===\\\\\n   -=|  %s\n===//\n", str);
}

int main(int argc, char** argv) {
    if (argc < 1)
        return 134;
    if (argc < 3) {
        usage(argv[0]);
    }
    try {
        std::string conn_addr_targ = argv[1];
        std::string msg;
        for (int i = 2; i < argc; i++) {
            if (!msg.empty())
                msg += '\n';
            msg += argv[i];
        }
        int ret;
        een9::SocketAddressParser sap;
        een9::SocketAddress addr;
        ret = een9::parse_socket_address(conn_addr_targ, addr, sap);
        een9_ASSERT(ret == 0, "Incorrect address");
        int sock = socket(addr.v.gen.sa_family, SOCK_STREAM, 0);
        een9_ASSERT_on_iret(sock, "creating socket to target server");
        een9::UniqueFdWrapper sockGuard(sock);
        een9::connect_to_socket_address(sock, addr);
        funny_log_print("Ready to send request");
        send_request_msg(sock, msg);
        funny_log_print("Admin-cmd request has been sent");
        std::string answer = receive_response_msg(sock);
        funny_log_print("Admin-cmd response has been read");
        printf("Output:\n%s", answer.c_str());
    } catch (const std::exception& e) {
        printf("%s\n", e.what());
    }
}