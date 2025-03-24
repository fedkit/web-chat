#include "socket_address.h"
#include <stddef.h>
#include "baza.h"
#include "baza_inter.h"
#include <libregexis024tools/delayed_matching.h>
#include <libregexis024tools/stringmatching.h>
#include <libregexis024vm/libregexis024vm_interface.h>
#include <vector>
#include <assert.h>
#include <algorithm>

namespace een9 {
    struct regexp_cmp_out{
        std::vector<uint8_t> prg;
        regexis024::track_var_list vars;
    };
    regexp_cmp_out compile_regexp(const char* expr) {
        regexp_cmp_out res;
        std::string problem;
        int ret = regexis024::compile(expr, res.vars, res.prg, problem);
        ASSERT(ret == 0, "Can't compile regexp");
        return res;
    }

    struct SocketAddressParser_Inner {
        regexp_cmp_out prg;
        regexis024::VirtualMachine vm;
        regexis024::tai_t tdiff_k;
        std::pair<regexis024::tai_t, regexis024::tai_t> i4_k;
        std::pair<regexis024::tai_t, regexis024::tai_t> i6_k;
        std::pair<regexis024::tai_t, regexis024::tai_t> ip_k;
        regexis024::tai_t skip_k;

#define reg_int4 "#4(0|[1-9][0-9]!r{0;2})"
#define reg_int6 "#6(0|[1-9a-fA-F][0-9a-fA-F]!r{0;3})"
#define reg_6RHT_rep(mt) "(:|(:" reg_int6 ")!r{1;" #mt "})"
#define reg_6LFT_rep(ea) "(" reg_int6 ":)!r{" #ea "}"
#define reg_6zs "#s:0;"
#define reg_intp "#p(0|[1-9][0-9]!r{0;4})"
#define reg_addr_in4 "(#t:1;" reg_int4 "." reg_int4 "." reg_int4 "." reg_int4 ":" reg_intp ")"
#define reg_addr_in6_core "(" reg_6LFT_rep(7) reg_int6 "|" ":" reg_6zs reg_6RHT_rep(6) "|" \
    reg_6LFT_rep(1) reg_6zs reg_6RHT_rep(5) "|" \
    reg_6LFT_rep(2) reg_6zs reg_6RHT_rep(4) "|" \
    reg_6LFT_rep(3) reg_6zs reg_6RHT_rep(3) "|" \
    reg_6LFT_rep(4) reg_6zs reg_6RHT_rep(2) "|" \
    reg_6LFT_rep(5) reg_6zs reg_6RHT_rep(1) "|" \
    reg_6LFT_rep(6) reg_6zs ":" ")"
#define reg_addr_in6 "(#t:2;\\["  reg_addr_in6_core "\\]:" reg_intp ")"
        SocketAddressParser_Inner(): prg(compile_regexp(
            reg_addr_in4 "|" reg_addr_in6
        )), vm(prg.prg.size(), prg.prg.data(), UINT64_MAX, UINT16_MAX, UINT32_MAX, UINT32_MAX, UINT64_MAX)
        {
            ASSERT_pl(vm.initialize() == 0);
            tdiff_k = prg.vars.at("t").colarr_first;
            skip_k = prg.vars.at("s").colarr_first;
            auto obtain_range = [&](const std::string& name) -> std::pair<regexis024::tai_t, regexis024::tai_t> {
                const regexis024::TrackingVariableInfo& vi = prg.vars.at(name);
                assert(vi.colarr_first > 0 && vi.colarr_second > 0);
                return {(regexis024::tai_t)vi.colarr_first, (regexis024::tai_t)vi.colarr_second};
            };
            i4_k = obtain_range("4");
            i6_k = obtain_range("6");
            ip_k = obtain_range("p");
        }
    };

    SocketAddressParser::SocketAddressParser() {
        opaque = new SocketAddressParser_Inner();
    }

    SocketAddressParser::~SocketAddressParser() {
        delete (SocketAddressParser_Inner*)opaque;
    }

    int parse_socket_address(const std::string& addr, SocketAddress& res, SocketAddressParser& pdata) {
#define reveal (*(SocketAddressParser_Inner*)pdata.opaque)
        reveal.vm.wipeToInit();
        ASSERT_pl(reveal.vm.addNewMatchingThread() == 0);
        int ret;
        for (size_t i = 0; i < addr.size(); i++) {
            ret = reveal.vm.feedCharacter((uint64_t)addr[i], 1);
            // if (!reveal.vm.haveSurvivors()) {
                // printf("DEBUG: died on %s\n", addr.substr(0, i + 1).c_str());
                // break;
            // }
        }
        if (reveal.vm.isMatched()) {
            std::vector<regexis024::CAEvent> ca = reveal.vm.getMatchedThreadCABranchReverse();
            std::reverse(ca.begin(), ca.end());
            size_t ci = 0;
#define curKey() ca[ci].key
#define curValue() ca[ci].value
            auto extractRange = [&](std::pair<regexis024::tai_t, regexis024::tai_t> rk) -> std::string {
                assert(ca[ci].key == rk.first && ca[ci + 1].key == rk.second);
                size_t oci = ci;
                ci += 2;
                return getSubstring(addr, ca[oci].value, ca[oci + 1].value);
            };
            assert(curKey() == reveal.tdiff_k);
            if (curValue() == 1) {
                ci++;
                uint32_t res_addr = 0;
                for (int i = 0; i < 4; i++) {
                    std::string h = extractRange(reveal.i4_k);
                    uint32_t p = std::stoul(h);
                    if (p > 255)
                        return -1;
                    res_addr = ((res_addr << 8) | p);
                }
                uint32_t res_port = std::stoul(extractRange(reveal.ip_k));
                if (res_port > 65535)
                    return -1;
                res.v.gen.sa_family = AF_INET;
                res.v.sin.sin_port = htons(res_port);  // host to network short
                res.v.sin.sin_addr.s_addr = htonl(res_addr); // host to network long
                res.addrLen = sizeof(sockaddr_in);
            } else if (curValue() == 2){
                ci++;
                int skipped = 8;
                for (const regexis024::CAEvent& ev: ca) {
                    if (ev.key == reveal.i6_k.first)
                        skipped--;
                }
                assert(skipped == 0 || skipped >= 2);
                uint16_t res_u6_addr16[8];
                size_t bi = 0;
                std::string h;
                while (bi < 8) {
                    if (curKey() == reveal.i6_k.first) {
                        h = extractRange(reveal.i6_k);
                        assert(h.size() <= 4);
                        uint32_t v = 0;
                        for (char ch: h) {
                            v <<= 4;
                            if ('0' <= ch && ch <= '9') {
                                v |= (uint32_t)(ch - '0');
                            } else if ('a' <= ch && ch <= 'z') {
                                v |= (uint32_t)(ch - 'a' + 10);
                            } else if ('A' <= ch && ch <= 'Z') {
                                v |= (uint32_t)(ch - 'A' + 10);
                            } else
                                assert(false);
                        }
                        assert(v <= UINT16_MAX);
                        res_u6_addr16[bi++] = (uint16_t)v;
                    } else if (curKey() == reveal.skip_k) {
                        ci++;
                        for (int j = 0; j < skipped; j++)
                            res_u6_addr16[bi++] = 0;
                    } else
                        assert(false);
                }
                assert(bi == 8);
                uint32_t res_port = std::stoul(extractRange(reveal.ip_k));
                if (res_port > 65535)
                    return -1;
                res.v.gen.sa_family = AF_INET6;
                res.v.sin6.sin6_port = htons(res_port);
                for (int i = 0; i < 8; i++)
                    res.v.sin6.sin6_addr.__in6_u.__u6_addr16[i] = htons(res_u6_addr16[i]);
                res.addrLen = sizeof(sockaddr_in6);
            } else
                assert(false);
            assert(ci == ca.size());
            return 0;
        }
        const std::string& up = "unix:";
        if (beginsWith(addr, up)) {
            std::string path = addr.substr(up.size());
            if (path.empty())
                return -1;
            if (path.back() == '/')
                return -1;
            for (char ch: path)
                if (ch == 0)
                    return -1;
            res.v.gen.sa_family = AF_UNIX;
            if (sizeof(res.v.sun.sun_path) < path.size())
                THROW("path is too big");
            memcpy(res.v.sun.sun_path, path.c_str(), path.size());
            res.addrLen = offsetof(sockaddr_un, sun_path) + path.size();
            return 0;
        }
        return -1;
    }

    std::string short2hex(uint16_t v) {
        if (v == 0)
            return "0";
        std::string result;
        while (v) {
            result += (char)((v & 0xf) > 9 ? (v & 0xf) - 10 + 'a' : (v & 0xf) + '0');
            v >>= 4;
        }
        std::reverse(result.begin(), result.end());
        return result;
    }

    std::string stringify_socket_address(const SocketAddress &addr) {
        if (addr.v.gen.sa_family == AF_INET) {
            char buf[22];
            uint32_t IP = ntohl(addr.v.sin.sin_addr.s_addr);
            uint16_t port = ntohs(addr.v.sin.sin_port);
            snprintf(buf, 22, "%u.%u.%u.%u:%hu", (IP >> 24) & 0xff, (IP >> 16) & 0xff, (IP >> 8) & 0xff,
                (IP >> 0) & 0xff, port);
            return buf;
        } else if (addr.v.gen.sa_family == AF_INET6) {
            uint16_t IP[8];
            for (int i = 0; i < 8; i++)
                IP[i] = ntohs(addr.v.sin6.sin6_addr.__in6_u.__u6_addr16[i]);
            uint16_t port = ntohs(addr.v.sin6.sin6_port);
            int largest_sz = 0;
            int largest_start = 0;
            int cur_sz = 0;
            for (int i = 0; i < 8; i++) {
                if (IP[i] == 0) {
                    cur_sz++;
                    if (largest_sz < cur_sz) {
                        largest_sz = cur_sz;
                        largest_start = i + 1 - cur_sz;
                    }
                } else {
                    cur_sz = 0;
                }
            }
            std::string core;
            for (int i = 0; i < 8;) {
                if (largest_sz >= 2 && largest_start == i) {
                    i += largest_sz;
                    if (i == 8)
                        core += "::";
                    else
                        core += ":";
                } else {
                    if (i > 0)
                        core += ":";
                    core += short2hex(IP[i]);
                    i++;
                }
            }
            assert(core.size() <= 39);
            char buf[48];
            snprintf(buf, 48, "[%s]:%hu", core.c_str(), port);
            return buf;
        } else if (addr.v.gen.sa_family == AF_UNIX) {
            assert(addr.addrLen > offsetof(sockaddr_un, sun_path));
            size_t path_len = addr.addrLen - offsetof(sockaddr_un, sun_path);
            assert(path_len <= sizeof(addr.v.sun.sun_path));
            std::string path(path_len, ')');
            memcpy(path.data(), addr.v.sun.sun_path, path_len);
            return "unix:" + path;
        } else
            return "Socket address of unknown domain";
    }

    void bind_to_socket_address(int sockfd, const SocketAddress &addr) {
        sa_family_t f = addr.v.gen.sa_family;
        if (f == AF_INET || f == AF_INET6 || f == AF_UNIX) {
            int ret = bind(sockfd, &addr.v.gen, addr.addrLen);
            ASSERT_on_iret(ret, "binding socket");
        } else
            THROW("binding socket to address of unsupported domain");
    }

    void get_peer_name_as_socket_address(int sockfd, SocketAddress &res) {
        socklen_t willbecome = sizeof(res.v);
        int ret = getpeername(sockfd, &res.v.gen, &willbecome);
        ASSERT_on_iret(ret, "getpeername");
        assert(willbecome <= sizeof(res.v));
        res.addrLen = willbecome;
    }

    void connect_to_socket_address(int sockfd, const SocketAddress& targ) {
        int ret = connect(sockfd, &targ.v.gen, targ.addrLen);
        ASSERT_on_iret(ret, "connect socket to addr");
    }
}
