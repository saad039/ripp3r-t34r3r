#if !defined(__LOAD_BALANCER__)
#define __LOAD_BALANCER__
#include "util.hpp"
#include <cassert>
#include <iterator>
#include <optional>
#include <tuple>
#include <vector>

template <typename InputIterator>
class load_balancer {

    typedef typename std::iterator_traits<InputIterator>::value_type value_type;
    typedef typename std::iterator_traits<InputIterator>::pointer pointer_type;
    typedef typename std::iterator_traits<InputIterator>::difference_type diff_type;

    const pointer_type start;
    const pointer_type end;
    const diff_type total_load_size;

    const std::size_t nprocs;

    // std::size_t per_each = total_load_size / (nprocs - 1);

    // std::size_t remainder = total_load_size - (per_each * nprocs);

    std::size_t per_each;
    std::size_t remainder;

  public:
    constexpr load_balancer(InputIterator first, InputIterator last, std::size_t nprocs) noexcept
        : start(first), end(last), total_load_size(std::distance(first, last)), nprocs(nprocs) {
        per_each = nprocs > 1 ? total_load_size / nprocs : 0; // 26/2 = 13
        remainder =
            nprocs > 1 ? total_load_size - per_each * (nprocs - 1) : total_load_size; // 26 - 13 =13
    }

    // constexpr diff_type size() const noexcept { return total_load_size; }

    auto get_nodes_work_chunk() const noexcept -> std::vector<std::vector<value_type>> {
        auto begin = start;
        std::vector<std::vector<value_type>> chunks(nprocs - 1);

        for (std::size_t i = 0; i < (nprocs - 1) * per_each; i++)
            chunks[i / per_each].push_back(*begin++);

        return chunks;
    }

    auto remainder_work() const noexcept -> std::optional<std::vector<value_type>> {
        if (!remainder) return std::nullopt;
        return std::optional(std::vector<value_type>(start + (nprocs - 1) * per_each, end));
    }

    decltype(auto) divide_work(bool balance = false) const noexcept {
        assert((std::size_t)total_load_size >= nprocs);
        auto nodes_work = get_nodes_work_chunk();
        auto leftover_work = remainder_work();
        if (balance and nprocs > 1) {
            if (leftover_work and leftover_work.value().size() > nodes_work[0].size()) {
                std::size_t i = 0;
                auto& target = leftover_work.value();
                while (target.size() != per_each) {
                    nodes_work[i++].push_back(*target.begin());
                    target.erase(target.begin());
                }
            }
        }
        return std::make_pair(nodes_work, leftover_work);
    }
};

// nprocs  each_node   remaining
// 1           26          0
// 2           13          0
// 3           8           2
// 4           6           2
// 5           5           1
// 6           4           2
// 7           3           5
// 8           3           2
// 9           2           8
// 10          2           6
// 11          2           4
// 12          2           4
// 13          2           0
// 14          1           12
// 15          1           11
// 16          1           10
// 17          1           9
// 18          1           8
// 19          1           7
// 20          1           6
// 21          1           5
// 22          1           4
// 23          1           3
// 24          1           2
// 25          1           1
// 26          1

#endif // __LOAD_BALANCER__
