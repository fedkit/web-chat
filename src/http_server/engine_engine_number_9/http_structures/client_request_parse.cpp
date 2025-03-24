#include "client_request_parse.h"
#include "../baza_inter.h"
#include <libregexis024tools/delayed_matching.h>
#include <algorithm>
#include <assert.h>
// Used for debug

#ifdef DEBUG_ALLOW_LOUD
#include "unistd.h"
#include <sys/stat.h>
#include "sys/dir.h"
#include "../os_utils.h"
#endif

namespace een9 {
    ClientRequestParser_CommonPrograms::ClientRequestParser_CommonPrograms() {
        regexis024::track_var_list vars;
        std::string emsg;
#define reg_ALPHA "[a-zA-Z]"
#define reg_pchar "([a-zA-Z0-9\\-._~\\!$\\&'()*+@:,;=]|%[0-9a-hA-H]!r{2})"
#define reg_query "(" reg_pchar"|[?/])*"
#define reg_lin_ws "([ \t]|\r\n[ \t])*"
#define reg_request_line "#method(" reg_ALPHA"+) #uri_path(/(" reg_pchar"|/)*)(\\?#uri_query(" reg_query"))? HTTP/#http_version(!digit;+.!digit;+)\r\n"
#define reg_filed_value "(" reg_lin_ws"#header_field_value_part([\\u0021-\\u007e&^\r\n]+))*" reg_lin_ws
#define reg_HTTP_message reg_request_line "(#header_field_name([\\u0021-\\u007E&^:]+):" reg_filed_value "\r\n)*\r\n"
        int ret = compile(reg_HTTP_message, vars, http_request_parse_prg, emsg);
        ASSERT(ret >= 0, "regexis024::compile. " + emsg);
#define retrieve_variable(name) ASSERT_pl(vars.count(#name) > 0); ASSERT_pl(vars[#name].colarr_first >= 0); \
    ASSERT_pl(vars[#name].colarr_second >= 0); name ## _beg = vars[#name].colarr_first; name ## _end = vars[#name].colarr_second;
        retrieve_variable(method);
        retrieve_variable(uri_path);
        retrieve_variable(uri_query);
        retrieve_variable(http_version);
        retrieve_variable(header_field_name);
        retrieve_variable(header_field_value_part);
    }

    ClientRequestParser_WorkerBuffers::ClientRequestParser_WorkerBuffers(
        const ClientRequestParser_CommonPrograms &common_comp_program
    ): http_request_parse_vm(
        common_comp_program.http_request_parse_prg.size(), common_comp_program.http_request_parse_prg.data(),
        UINT64_MAX, UINT16_MAX, UINT32_MAX, UINT32_MAX, UINT64_MAX)
    {
        ASSERT_pl(http_request_parse_vm.initialize() == 0);
    }

    ClientHttpRequestParser_Ctx::ClientHttpRequestParser_Ctx(
        ClientRequest &res, ClientRequestParser_WorkerBuffers &wb, ClientRequestParser_CommonPrograms& cp
    ): res(res), vm(wb.http_request_parse_vm), cp(cp)
    {
        vm.wipeToInit();
        ASSERT_pl(vm.addNewMatchingThread() == 0);
    }

    int ClientHttpRequestParser_Ctx::feedCharacter(char ch) {
        assert(status == 0);
        if (collecting_body) {
            res.body += ch;
            if (res.body.size() >= body_size) {
                status = 1;
            }
        } else {
            header += ch;
            if (vm.feedCharacter(ch, 1) < 0) {
                THROW("vm error");
            }
            if (vm.isMatched()) {
                /* Finishing line */
                std::vector<regexis024::CAEvent> ca = vm.getMatchedThreadCABranchReverse();
                std::reverse(ca.begin(), ca.end());
                size_t cur_ca_i = 0;
                auto getCaV = [&](ssize_t offset) -> uint64_t { return ca[cur_ca_i + offset].value; };
                auto getCaK = [&](ssize_t offset) -> regexis024::tai_t { return ca[cur_ca_i + offset].key; };
                auto isThat = [&](ssize_t offset, regexis024::tai_t key) -> bool {
                    return ca.size() > cur_ca_i + offset && getCaK(offset) == key;
                };
#define vibe_check(boff, name) isThat(boff, cp.name ## _beg) && isThat(boff + 1, cp.name ## _end)
                ASSERT_pl(vibe_check(0, method) && vibe_check(2, uri_path));
                res.method = getSubstring(header, getCaV(0), getCaV(1));
                res.uri_path = getSubstring(header, getCaV(2), getCaV(3));
                cur_ca_i += 4;
                if (isThat(0, cp.uri_query_beg)) {
                    ASSERT_pl(vibe_check(0, uri_query));
                    res.has_query = true;
                    res.uri_query = getSubstring(header, getCaV(0), getCaV(1));
                    cur_ca_i += 2;
                }
                ASSERT_pl(vibe_check(0, http_version));
                res.http_version = getSubstring(header, getCaV(0), getCaV(1));
                cur_ca_i += 2;
                while (isThat(0, cp.header_field_name_beg)) {
                    ASSERT_pl(vibe_check(0, header_field_name));
                    std::string field_name = getSubstring(header, getCaV(0), getCaV(1));
                    cur_ca_i += 2;
                    std::string field_value;
                    while (isThat(0, cp.header_field_value_part_beg)) {
                        ASSERT_pl(vibe_check(0, header_field_value_part));
                        if (!field_value.empty())
                            field_value += " ";
                        field_value += getSubstring(header, getCaV(0), getCaV(1));
                        cur_ca_i += 2;
                    }
                    res.headers.emplace_back(field_name, field_value);
                }
                /* Finished header processing */
                for (auto& p: res.headers) {
                    if (p.first == "Content-Length") {
                        collecting_body = res.has_body = true;
                        body_size = std::stoull(p.second);
                        if (body_size > 100000000) {
                            status = -1;
                            return status;
                        }
                        res.body.reserve(std::min(100000ul, body_size));
                        if (body_size == 0) {
                            status = 1;
                        }
                        break;
                    }
                }
                if (!res.has_body) {
                    status = 1;
                }
                /* We either finish now or we finish later */
            } else if (!vm.haveSurvivors()) {
#ifdef DEBUG_ALLOW_LOUD
                mkdir("log", 0750);
                writeFile("log/req", header);
#endif
                status = -1;
            }
        }
        return status;
    }
}
