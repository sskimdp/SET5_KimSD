#include <iostream>
#include <vector>
#include <string>
#include <unordered_set>
#include <algorithm>
#include <cmath>
#include <cstdint>

#include "RandomStreamGen.cpp"
#include "HashFuncGen.cpp"
#include "HyperLogLog.cpp"

static inline int clz32(std::uint32_t x) {
    return __builtin_clz(x);
}

static inline std::uint8_t rho32(std::uint32_t w, int L) {
    if (w == 0) return static_cast<std::uint8_t>(L + 1);
    int r = clz32(w) + 1;
    if (r > L + 1) r = L + 1;
    return static_cast<std::uint8_t>(r);
}

struct RegStats {
    double mean = 0.0;
    double std = 0.0;
    std::size_t mn = 0;
    std::size_t mx = 0;
};

static RegStats regStats(const std::vector<std::size_t>& cnt) {
    double m = 0.0;
    for (auto v : cnt) m += (double)v;
    m /= (double)cnt.size();

    double var = 0.0;
    std::size_t mn = cnt[0], mx = cnt[0];
    for (auto v : cnt) {
        mn = std::min(mn, v);
        mx = std::max(mx, v);
        double d = (double)v - m;
        var += d * d;
    }
    var /= (double)cnt.size();
    return {m, std::sqrt(var), mn, mx};
}

static double rseTheory(int B) {
    double m = (double)(1u << B);
    return 1.04 / std::sqrt(m);
}

int main() {
    const std::size_t N = 200000;
    const std::uint64_t streamSeed = 42;
    const std::uint64_t hashMasterSeed = 777;

    RandomStreamGen::Config cfg;
    cfg.seed = streamSeed;

    RandomStreamGen gen(cfg);
    auto stream = gen.generate(N);

    std::unordered_set<std::string> uniq(stream.begin(), stream.end());
    std::vector<std::string> data;
    data.reserve(uniq.size());
    for (auto& s : uniq) data.push_back(s);

    HashFuncGen hgen(hashMasterSeed);
    auto h = hgen.make();

    std::cout << "Total:  " << stream.size() << "\n";
    std::cout << "Unique: " << data.size() << "\n\n";

    std::vector<int> Bs = {10, 12, 14, 16};

    for (int B : Bs) {
        std::size_t m = 1u << B;
        int L = 32 - B;

        std::vector<std::size_t> regCnt(m, 0);
        std::vector<std::size_t> rhoCnt(L + 2, 0);

        for (const auto& s : data) {
            std::uint32_t x = h(s);

            std::uint32_t idx = x >> (32 - B);
            regCnt[idx]++;

            std::uint32_t w = x << B;
            std::uint8_t r = rho32(w, L);
            rhoCnt[r]++;
        }

        auto st = regStats(regCnt);

        std::cout << "=== B=" << B << " (m=" << m << "), theory RSE ~ "
                  << (rseTheory(B) * 100.0) << "% ===\n";
        std::cout << "Registers: mean=" << st.mean
                  << ", std=" << st.std
                  << ", min=" << st.mn
                  << ", max=" << st.mx << "\n";

        std::cout << "rho distribution (first values):\n";
        double total = (double)data.size();
        for (int k = 1; k <= std::min(6, L+1); ++k) {
            double obs = (double)rhoCnt[k];
            double exp = total * std::pow(0.5, k);
            std::cout << "  rho=" << k
                      << ": obs=" << obs
                      << ", exp~" << exp
                      << ", obs/exp~" << (exp > 0 ? obs/exp : 0.0)
                      << "\n";
        }
        std::cout << "\n";
    }

    return 0;
}
