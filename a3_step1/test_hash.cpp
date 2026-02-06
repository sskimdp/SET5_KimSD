#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <unordered_set>

#include "RandomStreamGen.cpp"
#include "HashFuncGen.cpp"

struct UniformityStats {
    double mean = 0.0;
    double variance = 0.0;
    std::size_t minCount = 0;
    std::size_t maxCount = 0;
};

UniformityStats testUniformity(const HashFunc& h,
                              const std::vector<std::string>& stream,
                              std::size_t binsPow2 = 16) {
    std::size_t bins = 1ULL << binsPow2;
    std::vector<std::size_t> cnt(bins, 0);

    // берём только уникальные строки
    std::unordered_set<std::string> uniq(stream.begin(), stream.end());

    std::uint32_t mask = static_cast<std::uint32_t>(bins - 1);
    for (const auto& s : uniq) {
        std::uint32_t x = h(s);
        cnt[(x >> 16) & mask] += 1;
    }

    double samples = static_cast<double>(uniq.size());
    double mean = samples / static_cast<double>(bins);

    double var = 0.0;
    std::size_t mn = cnt[0], mx = cnt[0];
    for (std::size_t v : cnt) {
        mn = std::min(mn, v);
        mx = std::max(mx, v);
        double d = static_cast<double>(v) - mean;
        var += d * d;
    }
    var /= static_cast<double>(bins);

    return {mean, var, mn, mx};
}

int main() {
    RandomStreamGen::Config cfg;
    cfg.seed = 42;
    RandomStreamGen gen(cfg);

    auto stream = gen.generate(200000);

    HashFuncGen hgen(777);
    auto h = hgen.make();

    std::unordered_set<std::string> uniq(stream.begin(), stream.end());
    std::cout << "Total strings:  " << stream.size() << "\n";
    std::cout << "Unique strings: " << uniq.size() << "\n";

    auto st = testUniformity(h, stream, 16);
    std::cout << "Mean per bin: " << st.mean << "\n";
    std::cout << "Var per bin: " << st.variance << "\n";
    std::cout << "Min count:   " << st.minCount << "\n";
    std::cout << "Max count:   " << st.maxCount << "\n";
}
