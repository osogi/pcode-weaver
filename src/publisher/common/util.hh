#include <vector>
#include <algorithm>


namespace util {
template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
} // namespace util


template <typename T, typename Func>
auto map_vector(const std::vector<T>& source, Func&& func) {
    // Deduce the return type of the function to create the correct vector type
    using ResultType = std::invoke_result_t<Func, const T&>;
    std::vector<ResultType> result;
    
    if (source.empty()) {
        return result;
    }

    result.reserve(source.size());

    std::transform(source.begin(), source.end(),
                   std::back_inserter(result),
                   std::forward<Func>(func));

    return result;
}