#ifndef NEW_YORK_TRANSIT_LINE_ALOTALOT_H
#define NEW_YORK_TRANSIT_LINE_ALOTALOT_H

#include <stdexcept>
#include <memory>

/* A little of this, a little of that
 * DO NOT EXPORT THIS FILE */

namespace nytl {
    template<typename T>
    using uptr = std::unique_ptr<T>;


    template<typename Tp>
    constexpr std::remove_reference_t<Tp>&&
    mv(Tp&& t) noexcept
    { return static_cast<std::remove_reference_t<Tp>&&>(t); }

    class FUp : public std::exception{
        std::string WHAT;
    public:
        FUp(const std::string &err, const std::string &file, const std::string &func, int line);

        const char *what() const noexcept override;
    };

    std::string prettyprint_errno(const std::string& pref);
#define THROW(err) throw FUp(err, __FILE__, __func__, __LINE__)
#define THROW_on_errno(err) THROW(prettyprint_errno(err))
#define ASSERT(cond, err) do { if (!(cond)) { THROW(err); } } while (0);
#define ASSERT_pl(cond) ASSERT(cond, "Failed assertion `" #cond "`")
#define ASSERT_on_iret(iret, err) ASSERT((iret) >= 0, prettyprint_errno(err));

    bool endsIn(const std::string& a, const std::string& b);

    std::string throwout_postfix(const std::string& a, size_t bsz);

    bool isALPHA(char ch);
    bool isNUM(char ch);
    bool isUNCHAR(char ch);
    bool isUNCHARnonNUM(char ch);
    bool isSPACE(char ch);

    bool isUname(const std::string& str);
    bool is_uname_dotted_sequence(const std::string& uinp);

    std::string make_uppercase(const std::string& source);

    void rstrip(std::string& str);
}

#endif
