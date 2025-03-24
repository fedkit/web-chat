#include "str_fields.h"
#include <jsonincpp/utf8.h>
#include <engine_engine_number_9/baza_throw.h>

namespace iu9cawebchat {
    bool isALPHA(char ch) {
        return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z');
    }

    bool isNUM(char ch) {
        return '0' <= ch && ch <= '9';
    }

    bool isUNCHAR(char ch) {
        return isALPHA(ch) || isNUM(ch) || ch == '-' || ch == '_';
    }

    bool isSPACE(char ch) {
        return ch == ' ' || ch == '\r' || ch == '\t' || ch == '\n';
    }


    bool is_orthodox_string(const std::string &str) {
        for (char ch: str)
            if (ch == 0)
                return false;
        return json::isUtf8String(str);
    }

    bool check_password(const std::string &pwd) {
        return is_orthodox_string(pwd) && pwd.size() <= 150;
    }

    bool check_strong_password(const std::string& pwd) {
        return check_password(pwd) && pwd.size() >= 8;
    }

    bool check_name(const std::string &name) {
        return is_orthodox_string(name) && name.size() <= 150;
    }

    bool check_nickname(const std::string &nickname) {
        if (nickname.empty())
            return false;
        for (char ch: nickname) {
            if (!isUNCHAR(ch))
                return false;
        }
        return nickname.size() <= 150;
    }

    /* Yeah baby, it's base64 time!!! */

    static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                    'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                    '4', '5', '6', '7', '8', '9', '+', '/'};

    static uint8_t decoding_table[256] = {
        69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69,
        69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69,
        69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 62, 69, 69, 69, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 69, 69, 69, 69, 69, 69,
        69, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 69, 69, 69, 69, 69,
        69, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 69, 69, 69, 69, 69,
        69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69,
        69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69,
        69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69,
        69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69,
        69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69,
        69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69,
        69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69,
        69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69,
    };

    std::string base64_encode(const std::string& source) {
        std::string result;
        static const size_t szr2noes[3] = {0, 2, 1};
        size_t noes = szr2noes[source.size() % 3];
        size_t triplets = source.size() / 3;
        result.reserve((triplets << 2) + noes);
        size_t bt = 0;
        for (size_t i = 0; i < triplets; i++) {
            result.push_back(encoding_table[(uint8_t)source[bt] >> 2]);
            result.push_back(encoding_table[(((uint8_t)source[bt] & 0x03) << 4) | ((uint8_t)source[bt + 1] >> 4)]);
            result.push_back(encoding_table[(((uint8_t)source[bt + 1] & 0x0f) << 2) | ((uint8_t)source[bt + 2] >> 6)]);
            result.push_back(encoding_table[(uint8_t)source[bt + 2] & 0x3f]);
            bt += 3;
        }
        if (noes == 1) {
            result.push_back(encoding_table[(uint8_t)source[bt] >> 2]);
            result.push_back(encoding_table[(((uint8_t)source[bt] & 0x03) << 4) | ((uint8_t)source[bt + 1] >> 4)]);

            result.push_back(encoding_table[((uint8_t)source[bt + 1] & 0x0f) << 2]);
            result.push_back('=');
        } else if (noes == 2) {
            result.push_back(encoding_table[(uint8_t)source[bt] >> 2]);

            result.push_back(encoding_table[((uint8_t)source[bt] & 0x03) << 4]);
            result.push_back('=');
            result.push_back('=');
        }
        return result;
    }

    std::string base64_decode(const std::string& source) {
#define myMsg "Bad base64 string. Can't decode"
        een9_ASSERT((source.size() & 0x3) == 0, myMsg);
        if (source.empty())
            return "";
        size_t fm = (source.size() >> 2) * 3;
        size_t noes = 0;
        if (*(source.end() - 2) == '=') {
            noes = 2;
            een9_ASSERT(source.back() == '=', myMsg);
        } else if (source.back() == '=')
            noes = 1;
        for (size_t i = 0; i + noes < source.size(); i++)
            een9_ASSERT(decoding_table[(uint8_t)source[i]] < 64, myMsg);
        std::string result;
        static const size_t noes2ab[3] = {0, 2, 1};
        result.reserve(fm + noes2ab[noes]);
        size_t naah = 0;
        for (; naah < source.size(); naah += 4) {
            result.push_back((char)((decoding_table[source[naah]] << 2) | (decoding_table[source[naah + 1]] >> 4)));
            if (naah + 4 == source.size() && noes == 2) {
                een9_ASSERT((decoding_table[source[naah + 1]] & 0x0f) == 0, myMsg);
                break;
            }
            result.push_back((char)(((decoding_table[source[naah + 1]] & 0x0f) << 4) | (decoding_table[source[naah + 2]] >> 2)));
            if (naah + 4 == source.size() && noes == 1) {
                een9_ASSERT((decoding_table[source[naah + 2]] & 0x03) == 0, myMsg);
                break;
            }
            result.push_back((char)(((decoding_table[source[naah + 2]] & 0x03) << 6) | decoding_table[source[naah + 3]]));
        }
        return result;
    }
}
