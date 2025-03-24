#include <engine_engine_number_9/baza_inter.h>
#include <engine_engine_number_9/http_structures/accept_language.h>

using namespace een9;

void test(const std::string& al, const std::vector<std::string>& rls) {
    std::vector<std::string> got = parse_header_Accept_Language(al);
    if (got != rls) {
        printf("Test failed: wrong answer\n");
        abort();
    }
    printf("Test passed\n");
}

void btest(const std::string& al) {
    try {
        parse_header_Accept_Language(al);
    } catch (std::exception& e) {}
    printf("...\n");
}

int main() {
    test("RU-RU, uk-EN; q = 12.22", {"uk-EN", "RU-RU"});
    test("   RU-RU   ,uk-EN; q = 12.22  ", {"uk-EN", "RU-RU"});
    test(" AAA; q=0.1,  BBB-bb ; q=3, *; q=3", {"BBB-bb", "", "AAA"});
    test(" AAA; q=0.1,  BBB-bb ; q=2.5, *; q=4.5", {"", "BBB-bb", "AAA"});
    test("ABB, AAA; q=0.1,AAB,  BBB-bb ; q=2.5, *; q=4.5", {"", "BBB-bb", "ABB", "AAB", "AAA"});
    test("", {});
    test("   ", {});
    btest(";;;;");
    btest(";;==;;");
    btest("-;");
    btest("-==");
    return 0;
}
