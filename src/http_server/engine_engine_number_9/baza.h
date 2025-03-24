#ifndef ENGINE_ENGINE_NUMBER_9_BAZA_H
#define ENGINE_ENGINE_NUMBER_9_BAZA_H

#include <string>
#include <memory>

namespace een9 {
    class ServerError : public std::exception{
        std::string WHAT;

    public:
        ServerError(const std::string &err, const std::string &file, const std::string &func, int line);

        const char *what() const noexcept override;
    };

    std::string prettyprint_errno(const std::string& pref);

    bool strIn(const std::string& str, const char* arr[]);

    // b is postfix of a
    bool endsWith(const std::string& a, const std::string& b);

    // b is prefix of a
    bool beginsWith(const std::string& a, const std::string& b);

    /* In case of error, throws een9::ServerError */
    std::string getSubstring(const std::string& str, size_t A, size_t B);

    std::string make_uppercase(const std::string &source);

    template<typename T>
    using uptr = std::unique_ptr<T>;
}

#endif
