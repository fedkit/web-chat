#ifndef ENGINE_ENGINE_NUMBER_9_SOCKET_ADDRESS_H
#define ENGINE_ENGINE_NUMBER_9_SOCKET_ADDRESS_H

#if defined(SOLARIS)
#include <netinet/in.h>
#endif
#include <netdb.h>
#include <arpa/inet.h>
#if defined(BSD)
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#include <sys/un.h>
#include <string>

namespace een9 {
    /* Right now een9 supports only IP4, IP6, unix domain. System service querying is not implemented yet *
     * */

    struct SocketAddress {
        union {
            sockaddr gen;
            sockaddr_in sin;
            sockaddr_in6 sin6;
            sockaddr_un sun;
        } v;
        size_t addrLen = sizeof(sockaddr_in);
    };

    /* Not thread-safe. Use only from one thread (at a time) */
    struct SocketAddressParser {
        void* opaque = NULL;

        SocketAddressParser();
        SocketAddressParser(const SocketAddressParser&) = delete;
        SocketAddressParser& operator=(const SocketAddressParser&) = delete;
        ~SocketAddressParser();
    };

    int parse_socket_address(const std::string& addr, SocketAddress& res, SocketAddressParser& pdata);

    std::string stringify_socket_address(const SocketAddress& addr);

    /* Throws ServerError on error */
    void bind_to_socket_address(int sockfd, const SocketAddress& addr);

    /* Throws ServerError on error */
    void get_peer_name_as_socket_address(int sockfd, SocketAddress& res);

    void connect_to_socket_address(int sockfd, const SocketAddress& targ);
}

#endif
