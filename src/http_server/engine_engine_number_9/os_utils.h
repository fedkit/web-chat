#ifndef ENGINE_ENGINE_NUMBER_9_OS_UTILS_H
#define ENGINE_ENGINE_NUMBER_9_OS_UTILS_H

#include "baza.h"

namespace een9 {
    class UniqueFdWrapper {
        int fd = -1;
    public:
        explicit UniqueFdWrapper() = default;
        explicit UniqueFdWrapper(int fd_);

        UniqueFdWrapper(const UniqueFdWrapper&) = delete;
        UniqueFdWrapper& operator=(const UniqueFdWrapper&) = delete;

        UniqueFdWrapper(UniqueFdWrapper&& formerOwner) noexcept;
        UniqueFdWrapper& operator=(UniqueFdWrapper&& formerOwner) noexcept;

        int operator()() const;

        ~UniqueFdWrapper();
    };

    bool isRegularFile(const std::string& path);

    bool isDirectory(const std::string& path);

    /* result += read(fd); Argument description is for error handling */
    void readFromFileDescriptor(int fd, std::string& result, const std::string& description = "");

    void readFile(const std::string& path, std::string& result);

    void writeFile(const std::string& path, const std::string& text);

    void configure_socket_rcvsndtimeo(int fd, timeval tv);
}

#endif
