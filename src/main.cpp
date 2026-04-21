#include <bits/stdc++.h>
using namespace std;

namespace sjtu {
using sv_t = std::string_view;

struct format_error : std::exception {
public:
    format_error(const char *msg = "invalid format") : msg(msg) {}
    auto what() const noexcept -> const char * override { return msg; }
private:
    const char *msg;
};

template <typename Tp>
struct formatter;

struct format_info {
    inline static constexpr auto npos = static_cast<std::size_t>(-1);
    std::size_t position;
    std::size_t consumed;
};

template <typename... Args>
struct format_string {
public:
    consteval format_string(const char *fmt);
    constexpr auto get_format() const -> std::string_view { return fmt_str; }
    constexpr auto get_index() const -> std::span<const format_info> { return fmt_idx; }
private:
    inline static constexpr auto Nm = sizeof...(Args);
    std::string_view fmt_str;
    std::array<format_info, Nm> fmt_idx;
};

consteval auto find_specifier(sv_t &fmt) -> bool {
    do {
        if (const auto next = fmt.find('%'); next == sv_t::npos) {
            return false;
        } else if (next + 1 == fmt.size()) {
            throw format_error{"missing specifier after '%'"};
        } else if (fmt[next + 1] == '%') {
            fmt.remove_prefix(next + 2);
        } else {
            fmt.remove_prefix(next + 1);
            return true;
        }
    } while (true);
};

template <typename Parser>
consteval void parse_one(sv_t &fmt, std::span<format_info> idx, std::size_t &n) {
    const auto last_pos = fmt.begin();
    if (!find_specifier(fmt)) {
        idx[n++] = { .position = format_info::npos, .consumed = 0 };
        return;
    }
    const auto position = static_cast<std::size_t>(fmt.begin() - last_pos - 1);
    const auto consumed = Parser::parse(fmt);
    idx[n++] = { .position = position, .consumed = consumed };
    if (consumed > 0) {
        fmt.remove_prefix(consumed);
    } else if (fmt.starts_with("_")) {
        fmt.remove_prefix(1);
    } else {
        throw format_error{"invalid specifier"};
    }
}

template <typename... Args>
consteval auto compile_time_format_check(sv_t fmt, std::span<format_info> idx) -> void {
    std::size_t n = 0;
    (parse_one<formatter<Args>>(fmt, idx, n), ...);
    if (find_specifier(fmt)) throw format_error{"too many specifiers"};
}

template <typename... Args>
consteval format_string<Args...>::format_string(const char *fmt) : fmt_str(fmt), fmt_idx() {
    compile_time_format_check<Args...>(fmt_str, fmt_idx);
}

template <typename StrLike>
    requires(
        std::same_as<StrLike, std::string> ||      \
        std::same_as<StrLike, std::string_view> || \
        std::same_as<StrLike, char *> ||           \
        std::same_as<StrLike, const char *>        \
    )
struct formatter<StrLike> {
    static constexpr auto parse(sv_t fmt) -> std::size_t {
        return fmt.starts_with("s") ? 1u : 0u;
    }
    static auto format_to(std::ostream &os, const StrLike &val, sv_t fmt = "s") -> void {
        if (fmt.starts_with("s") || fmt.starts_with("_")) {
            os << static_cast<sv_t>(val);
        } else {
            throw format_error{};
        }
    }
};

template <class T>
concept signed_integral_like = std::signed_integral<T> || (std::is_enum_v<T> && std::is_signed_v<std::underlying_type_t<T>>);
template <class T>
concept unsigned_integral_like = std::unsigned_integral<T> || (std::is_enum_v<T> && std::is_unsigned_v<std::underlying_type_t<T>>);

template <signed_integral_like T>
struct formatter<T> {
    static constexpr auto parse(sv_t fmt) -> std::size_t {
        return (fmt.starts_with("d") || fmt.starts_with("u")) ? 1u : 0u;
    }
    static auto format_to(std::ostream &os, const T &val, sv_t fmt = "_") -> void {
        if (fmt.starts_with("d")) {
            os << static_cast<long long>(val);
        } else if (fmt.starts_with("u")) {
            os << static_cast<unsigned long long>(val);
        } else if (fmt.starts_with("_")) {
            os << static_cast<long long>(val);
        } else {
            throw format_error{};
        }
    }
};

template <unsigned_integral_like T>
struct formatter<T> {
    static constexpr auto parse(sv_t fmt) -> std::size_t {
        return (fmt.starts_with("d") || fmt.starts_with("u")) ? 1u : 0u;
    }
    static auto format_to(std::ostream &os, const T &val, sv_t fmt = "_") -> void {
        if (fmt.starts_with("u")) {
            os << static_cast<unsigned long long>(val);
        } else if (fmt.starts_with("d")) {
            os << static_cast<long long>(val);
        } else if (fmt.starts_with("_")) {
            os << static_cast<unsigned long long>(val);
        } else {
            throw format_error{};
        }
    }
};

template <typename T>
struct is_vector : false_type {};
template <typename T, typename A>
struct is_vector<std::vector<T, A>> : true_type {};

template <typename T>
struct formatter<std::vector<T>> {
    static constexpr auto parse(sv_t fmt) -> std::size_t { return 0u; }
    static auto format_to(std::ostream &os, const std::vector<T> &v, sv_t fmt = "_") -> void {
        (void)fmt;
        os << '[';
        bool first = true;
        for (auto const &x : v) {
            if (!first) os << ',';
            first = false;
            formatter<T>::format_to(os, x, "_");
        }
        os << ']';
    }
};

template <typename... Args>
using format_string_t = format_string<std::decay_t<Args>...>;

template <typename... Args>
inline auto printf(format_string_t<Args...> fmt, const Args &...args) -> void {
    auto s = fmt.get_format();
    std::size_t i = 0;
    auto print_text_until_spec = [&](std::ostream &os){
        while (i < s.size()) {
            auto p = s.find('%', i);
            if (p == std::string_view::npos) {
                os << s.substr(i);
                i = s.size();
                break;
            }
            if (p + 1 < s.size() && s[p + 1] == '%') {
                os << s.substr(i, p - i + 1);
                i = p + 2;
                continue;
            }
            os << s.substr(i, p - i);
            i = p;
            break;
        }
    };
    std::ostringstream out;
    print_text_until_spec(out);
    auto handle_one = [&](auto const &value){
        if (i >= s.size() || s[i] != '%') return; // safety
        if (i + 1 >= s.size()) throw format_error{"missing specifier after '%'"};
        char c = s[i + 1];
        if (c == '%') { i += 2; print_text_until_spec(out); return; }
        if (c == 's') { formatter<std::decay_t<decltype(value)>>::format_to(out, value, "s"); }
        else if (c == 'd') { formatter<std::decay_t<decltype(value)>>::format_to(out, value, "d"); }
        else if (c == 'u') { formatter<std::decay_t<decltype(value)>>::format_to(out, value, "u"); }
        else if (c == '_') { formatter<std::decay_t<decltype(value)>>::format_to(out, value, "_"); }
        else { throw format_error{"invalid specifier"}; }
        i += 2; // consume "%x"
        print_text_until_spec(out);
    };
    (handle_one(args), ...);
    std::cout << out.str();
}

} // namespace sjtu

int main(){
    // No input/output required per problem; provide a dummy run to ensure binary builds.
    return 0;
}
