#ifndef ENGINE_ENGINE_NUMBER_9_HTTP_CLIENT_REQUEST_PARSE_H
#define ENGINE_ENGINE_NUMBER_9_HTTP_CLIENT_REQUEST_PARSE_H

/* Do not export this file */

#include "../baza.h"
#include "client_request.h"
#include <libregexis024vm/libregexis024vm_interface.h>

namespace een9 {
    /* One structure that contains regexp program and C.A.T. keys. All accesscan and should be read only */
    struct ClientRequestParser_CommonPrograms {
        std::vector<uint8_t> http_request_parse_prg;
        regexis024::tai_t method_beg;
        regexis024::tai_t method_end;
        regexis024::tai_t uri_path_beg;
        regexis024::tai_t uri_path_end;
        /* Splitting of query into components (with & and =) is defined in html spec, not in http spec */
        regexis024::tai_t uri_query_beg;
        regexis024::tai_t uri_query_end;
        regexis024::tai_t http_version_beg;
        regexis024::tai_t http_version_end;
        regexis024::tai_t header_field_name_beg;
        regexis024::tai_t header_field_name_end;
        regexis024::tai_t header_field_value_part_beg;
        regexis024::tai_t header_field_value_part_end;

        ClientRequestParser_CommonPrograms();
    };

    /* Many structures (one for each worker) that stores regexp machine that reads program from one common buffer
     * VM buffers should not be reallocateed between user requests. Note that after ClientRequestParser_CommonPrograms
     * has been destroyed, this vm should not be used */
    struct ClientRequestParser_WorkerBuffers {
        regexis024::VirtualMachine http_request_parse_vm;

        explicit ClientRequestParser_WorkerBuffers(const ClientRequestParser_CommonPrograms& common_comp_program);
    };

    struct ClientHttpRequestParser_Ctx {
        ClientRequest& res;
        regexis024::VirtualMachine& vm;
        ClientRequestParser_CommonPrograms& cp;
        /* 1 if reading has completed, 0 if reading can be continued, -1 if error occured (input is incorrect) */
        int status = 0;
        bool collecting_body = false;
        size_t body_size = 0;
        std::string header;

        ClientHttpRequestParser_Ctx(ClientRequest& res, ClientRequestParser_WorkerBuffers& wb, ClientRequestParser_CommonPrograms& cp);

        int feedCharacter(char ch);
    };
}

#endif
