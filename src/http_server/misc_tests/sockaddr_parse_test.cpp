#include <engine_engine_number_9/socket_address.h>
#include <engine_engine_number_9/baza.h>

using namespace een9;

void test(const std::string& test, bool is_correct, SocketAddressParser& parser) {
#define fup printf("Test failed\n"); fflush(stdout); abort(); return;
    SocketAddress addr;
    int ret = parse_socket_address(test, addr, parser);
    if ((ret == 0) != is_correct) {
        fup
    }
    if (is_correct) {
        std::string back = stringify_socket_address(addr);
        if (make_uppercase(test) != make_uppercase(back)){
            fup
        }
    }
    printf("Test passed\n");
}

void test_dcs(const std::string& test, const std::string& need, SocketAddressParser& parser) {
    SocketAddress addr;
    int ret = parse_socket_address(test, addr, parser);
    if (ret != 0) {
        fup
    }
    std::string right = stringify_socket_address(addr);
    if (right != need) {
        fup
    }
    printf("Test passed\n");
}

int main() {
    SocketAddressParser parser;
    test("127:0:0:1:1026", false, parser);
    test("[12::12:0:0:0]:600", true, parser);
    test("[12::12:0:FFFF:0]:600", true, parser);
    test("[12::11:1]:600", true, parser);
    test("[::a::]:600", false, parser);
    test("[FFd:1:1:1:1:FFd:1:f]:65535", true, parser);
    test("[f:f:f:f:f:0:1:f]:65535", true, parser);
    test("[1:1:1:1:1:0:1:0]:11212", true, parser);
    test("[1:1:1:1:1:1:1:1]:1", true, parser);
    test("[1:1:1:1:1:0:1:0]:65536", false, parser);
    test("[1:1:1:1:1:0:1:0]:65535", true, parser);
    test("[1:H:1:1:1:FFd:1:f]:65535", false, parser);

    test("[::]:1", true, parser);
    test("12.11.11.123:312", true, parser);
    test("0.1.111.123:31212", true, parser);
    test("0.1.111.123:65536", false, parser);
    test("0.1.111.123:65535", true, parser);
    test("0.1.111.255:65535", true, parser);
    test("255.0.255.255:65535", true, parser);
    test("255.0.256.0:65535", false, parser);
    test("255.0.1000.0:65535", false, parser);
    test("255.0.01.0:65535", false, parser);
    test("255.0.1.0:605535", false, parser);
    test("2.0.1.0:05535", false, parser);
    test("255.0.1.0::65535", false, parser);
    test(".255.0.1.0::65535", false, parser);
    test("..0.1.0::65535", false, parser);
    test("[FFFF0::]:0", false, parser);
    test("[0::01:]:0", false, parser);
    test("[0:fa:ffff::]:0", true, parser);
    test("[::a:a:a:a:a:a:b]:10", false, parser);
    test_dcs("[0:0:0:0:0:0:0:0]:413", "[::]:413", parser);
    test_dcs("[::0:0:0:0]:413", "[::]:413", parser);
    test_dcs("[::0:0:0:0:0:0]:413", "[::]:413", parser);
    test_dcs("[::a:a:0:0:0:0]:413", "[0:0:a:a::]:413", parser);
}