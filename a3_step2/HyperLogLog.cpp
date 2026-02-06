#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <stdexcept>

class HyperLogLog {
public:
    explicit HyperLogLog(int B)
        : B_(B), m_(1u << B), regs_(m_, 0) {
        if (B_ <= 0 || B_ >= 31) throw std::invalid_argument("B must be in [1..30]");
        alpha_ = alpha_m(m_);
        L_ = 32 - B_;
    }

    void reset() {
        std::fill(regs_.begin(), regs_.end(), 0);
    }

    void addHash(std::uint32_t x) {
        std::uint32_t idx = x >> (32 - B_);
        std::uint32_t w = x << B_;

        std::uint8_t r = rho(w, L_);
        regs_[idx] = std::max(regs_[idx], r);
    }

    double estimate() const {
        double Z = 0.0;
        int V = 0;
        for (std::uint8_t reg : regs_) {
            if (reg == 0) V++;
            Z += std::ldexp(1.0, -static_cast<int>(reg));
        }
        double E = alpha_ * (static_cast<double>(m_) * static_cast<double>(m_)) / Z;

        if (E <= 2.5 * m_ && V > 0) {
            E = static_cast<double>(m_) * std::log(static_cast<double>(m_) / V);
        }

        const double two32 = 4294967296.0; // 2^32
        if (E > two32 / 30.0 && E < two32) {
            E = -two32 * std::log(1.0 - E / two32);
        }
        return E;
    }

    int B() const { return B_; }
    std::uint32_t m() const { return m_; }

private:
    int B_;
    int L_;
    std::uint32_t m_;
    std::vector<std::uint8_t> regs_;
    double alpha_;

    static double alpha_m(std::uint32_t m) {
        if (m == 16) return 0.673;
        if (m == 32) return 0.697;
        if (m == 64) return 0.709;
        return 0.7213 / (1.0 + 1.079 / static_cast<double>(m));
    }

    static std::uint8_t rho(std::uint32_t w, int L) {
        if (w == 0) return static_cast<std::uint8_t>(L + 1);

        int r = __builtin_clz(w) + 1;
        if (r > L + 1) r = L + 1;
        return static_cast<std::uint8_t>(r);
    }
};
