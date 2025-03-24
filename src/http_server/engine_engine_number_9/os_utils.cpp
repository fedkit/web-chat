#include "os_utils.h"
#include <utility>
#include "baza_inter.h"
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>

namespace een9 {
    UniqueFdWrapper::UniqueFdWrapper(int fd_): fd(fd_) {}

    UniqueFdWrapper::UniqueFdWrapper(UniqueFdWrapper &&formerOwner) noexcept {
        fd = formerOwner.fd;
        formerOwner.fd = -1;
    }

    UniqueFdWrapper& UniqueFdWrapper::operator=(UniqueFdWrapper &&formerOwner) noexcept {
        std::swap(fd, formerOwner.fd);
        return *this;
    }

    int UniqueFdWrapper::operator()() const {
        return fd;
    }

    UniqueFdWrapper::~UniqueFdWrapper() {
        if (fd >= 0)
            close(fd);
    }

    bool isNeededFsEntity(const std::string &path, mode_t t_mode) {
        struct stat info;
        errno = 0;
        int ret = stat(path.c_str(), &info);
        if (errno == 0) {
            return (info.st_mode & S_IFMT) == t_mode;
        } if (errno == ENOENT)
            return false;
        THROW_on_errno("stat\"" + path + "\"");
    }

    bool isRegularFile(const std::string &path) {
        return isNeededFsEntity(path, S_IFREG);
    }

    bool isDirectory(const std::string &path) {
        return isNeededFsEntity(path, S_IFDIR);
    }

    void readFromFileDescriptor(int fd, std::string &result, const std::string &description) {
        int ret;
        char buf[2048];
        while ((ret = (int)read(fd, buf, 2048)) > 0) {
            size_t oldN = result.size();
            result.resize(oldN + ret);
            memcpy(result.data() + oldN, buf, ret);
        }
        ASSERT_on_iret(ret, "Reading from " + description);
    }

    void readFile(const std::string &path, std::string &result) {
        int fd = open(path.c_str(), O_RDONLY);
        ASSERT_on_iret(fd, "Opening \"" + path + "\"");
        UniqueFdWrapper fdw(fd);
        readFromFileDescriptor(fdw(), result, "file \"" + path + "\"");
    }


    /* write(fd, text); close(fd); */
    void writeToFileDescriptor(int fd, const std::string& text, const std::string& description = "") {
        size_t n = text.size();
        size_t i = 0;
        while (i < n) {
            size_t block = std::min(2048lu, n - i);
            int ret = write(fd, &text[i], block);
            ASSERT_on_iret(ret, "Writing to" + description);
            i += ret;
        }
        close(fd);
    }

    /* Truncational */
    void writeFile(const std::string& path, const std::string& text) {
        int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0755);
        ASSERT_on_iret(fd, "Opening \"" + path + "\"");
        UniqueFdWrapper fdw(fd);
        writeToFileDescriptor(fdw(), text, "file \"" + path + "\n");
    }

    void configure_socket_rcvsndtimeo(int fd, timeval tv) {
        int ret;
        ret = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(timeval));
        ASSERT_on_iret_pl(ret);
        ret = setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(timeval));
        ASSERT_on_iret_pl(ret);
    }
}