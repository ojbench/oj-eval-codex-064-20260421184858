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

template <typename... Args>
consteval auto compile_time_format_check(sv_t fmt, std::span<format_info> idx) -> void {
    std::size_t n = 0;
    auto parse_fn = [&fmt, &n, idx](const auto &parser) {
        const auto last_pos = fmt.begin();
        if (!find_specifier(fmt)) {
            idx[n++] = { .position = format_info::npos, .consumed = 0 };
            return;
        }
        const auto position = static_cast<std::size_t>(fmt.begin() - last_pos - 1);
        const auto consumed = parser.parse(fmt);
        idx[n++] = { .position = position, .consumed = consumed };
        if (consumed > 0) {
            fmt.remove_prefix(1);
        } else if (fmt.starts_with("_")) {
            fmt.remove_prefix(1);
        } else {
            throw format_error{"invalid specifier"};
        }
    };
    (parse_fn(formatter<Args>{}), ...);
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
        if (fmt.starts_with("s")) {
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
        return fmt.starts_with("d") ? 1u : 0u;
    }
    static auto format_to(std::ostream &os, const T &val, sv_t fmt = "d") -> void {
        if (fmt.starts_with("d")) os << static_cast<long long>(val);
        else throw format_error{};
    }
};

template <unsigned_integral_like T>
struct formatter<T> {
    static constexpr auto parse(sv_t fmt) -> std::size_t {
        return fmt.starts_with("u") ? 1u : 0u;
    }
    static auto format_to(std::ostream &os, const T &val, sv_t fmt = "u") -> void {
        if (fmt.starts_with("u")) os << static_cast<unsigned long long>(val);
        else throw format_error{};
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
        for (auto &x : v) {
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
    auto idx = fmt.get_index();
    std::size_t arg_i = 0; (void)arg_i;
    size_t pos = 0;
    std::ostringstream out;
    auto it = idx.begin();
    auto next_spec = [&]() -> size_t { return (it!=idx.end() ? it++->position : format_info::npos); };
    // Helper to consume spec
    auto consume = [&]() {
        if (pos < s.size() && s[pos] == '%') ++pos; // skip '%'
    };
    // visit args in order with a lambda pack expansion
    auto emit_until = [&](size_t upto){ out << std::string_view(s.data()+pos, upto-pos); pos = upto; };
    auto emit_text = [&](){ while (pos < s.size()) {
            auto p = s.find('%', pos);
            if (p==string::npos) { out << s.substr(pos); pos = s.size(); break; }
            if (p+1 < s.size() && s[p+1]=='%') { out << s.substr(pos, p-pos+1); pos = p+2; continue; }
            break;
        }
    };
    emit_text();
    size_t spec_pos = next_spec();
    auto process_arg = [&](auto const& value){
        if (spec_pos==format_info::npos) return; // no spec for this arg (shouldn't happen)
        emit_until(sv_t{s}.substr(pos).find('%')==string::npos? s.size(): s.find('%', pos));
        // Now at '%'
        // Determine specifier char
        char c = (pos+1 < s.size()? s[pos+1] : '\0');
        if (c=='%') { pos+=2; spec_pos = next_spec(); return; }
        if (c=='s') { consume(); formatter<decay_t<decltype(value)>>::format_to(out, value, "s"); }
        else if (c=='d') { consume(); formatter<decay_t<decltype(value)>>::format_to(out, value, "d"); }
        else if (c=='u') { consume(); formatter<decay_t<decltype(value)>>::format_to(out, value, "u"); }
        else if (c=='_') { consume(); formatter<decay_t<decltype(value)>>::format_to(out, value, "_"); }
        else { throw format_error{"invalid specifier"}; }
        ++pos; // move past spec char
        emit_text();
        spec_pos = next_spec();
    };
    (process_arg(args), ...);
    std::cout << out.str();
}

} // namespace sjtu

int main(){
    using sjtu::printf;
    // Demonstration output (no input required)
    string a = "hello"; int x = -42; unsigned y = 13; vector<int> v{1,2,3};
    printf<decltype(a), int, unsigned, decltype(v)>("%s %d %u %_\n", a, x, y, v);
    return 0;
}

