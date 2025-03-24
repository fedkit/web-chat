#include <iu9_ca_web_chat_lib/str_fields.h>
#include <random>
#include <stdexcept>
#include <engine_engine_number_9/baza.h>

int main() {
    auto test = [&](const std::string& octets) {
        std::string res = base64_encode(octets);
        std::string back = base64_decode(res);
        if (octets != back) {
            printf("Oshibka\n");
            abort();
        }
        printf("Test passed\n");
    };
    std::mt19937 mt(123);
    std::uniform_int_distribution<int> dist(0, 255);
    auto random_test = [&](int sz) {
        std::vector<int> octets(sz);
        std::string s0(sz, 0);
        printf("Test: ");
        for (int i = 0; i < sz; i++) {
            octets[i] = dist(mt);
            printf("%3d ", octets[i]);
            s0[i] = (char)(uint8_t)octets[i];
        }
        printf("\n");
        std::string got1 = base64_encode(s0);
        printf("Base64: %s\n", got1.c_str());
        std::string back2 = base64_decode(got1);
        if (s0 != back2) {
            printf("Test failed!\nBack: ");
            for (size_t i = 0; i < back2.size(); i++) {
                printf("%3d ", (int)(uint8_t)back2[i]);
            }
            printf("\n");
            abort();
        }
        printf("Test passed\n");
    };
    test("//");
    test("/0");
    test("/ ");
    test("  ");
    test("");
    test("1");
    test("22");
    test("333");
    test("4444");
    test("55555");
    test("666666");
    test("7777777");
    test("88888888");
    test("999999999");
    test("1010101010");
    test("11111111111");
    for (int i = 0; i < 100; i++) {
        for (int j = 0; j < 20; j++) {
            random_test(j);
        }
    }
    auto rb_test = [&](size_t sz) {
        std::string octets(sz, 0);
        for (int i = 0; i < sz; i++)
            octets[i] = (char)(uint8_t)dist(mt);
        try {
            std::string gr = base64_decode(octets);
            printf("Hoba, that was a good one\n");
        } catch (een9::ServerError& e) {
            printf("Finished with error\n");
        }
    };
    printf("Now it's time for robustness test\n");
    for (int j = 0; j < 16; j++) {
        for (int i = 0; i < j * j * j / 100 + j * j / 5 + j * 100; i++) {
            rb_test(i);
        }
    }
    return 0;
}
