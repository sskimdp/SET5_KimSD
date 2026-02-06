#pragma once
#include <string>
#include <vector>
#include <random>
#include <cstdint>
#include <fstream>
#include <stdexcept>

class RandomStreamGen {
public:
struct Config {
    std::uint64_t seed;
    std::size_t minLen;
    std::size_t maxLen;
    std::string alphabet;

    Config()
        : seed(123456789ULL),
          minLen(1),
          maxLen(30),
          alphabet("abcdefghijklmnopqrstuvwxyz"
                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                   "0123456789-") {}
};
    explicit RandomStreamGen(Config cfg = Config()) : cfg_(std::move(cfg)), rng_(cfg_.seed) {
        if (cfg_.minLen == 0 || cfg_.minLen > cfg_.maxLen || cfg_.maxLen > 30) {
            throw std::invalid_argument("RandomStreamGen: invalid length bounds");
        }
        if (cfg_.alphabet.empty()) {
            throw std::invalid_argument("RandomStreamGen: alphabet is empty");
        }
    }

    std::vector<std::string> generate(std::size_t N) {
        std::vector<std::string> stream;
        stream.reserve(N);

        std::uniform_int_distribution<std::size_t> lenDist(cfg_.minLen, cfg_.maxLen);
        std::uniform_int_distribution<std::size_t> chDist(0, cfg_.alphabet.size() - 1);

        for (std::size_t i = 0; i < N; ++i) {
            std::size_t L = lenDist(rng_);
            std::string s;
            s.reserve(L);
            for (std::size_t j = 0; j < L; ++j) {
                s.push_back(cfg_.alphabet[chDist(rng_)]);
            }
            stream.push_back(std::move(s));
        }
        return stream;
    }

    static std::vector<std::size_t> prefixSizesByPercent(std::size_t N, std::size_t stepPercent) {
        if (stepPercent == 0 || stepPercent > 100) {
            throw std::invalid_argument("stepPercent must be in [1..100]");
        }
        std::vector<std::size_t> sizes;
        for (std::size_t p = stepPercent; p <= 100; p += stepPercent) {
            std::size_t k = (N * p) / 100;
            if (k == 0) k = 1;
            if (k > N) k = N;
            sizes.push_back(k);
        }
        if (sizes.empty() || sizes.back() != N) sizes.push_back(N);
        return sizes;
    }

    static void saveToFile(const std::vector<std::string>& stream, const std::string& path) {
        std::ofstream out(path);
        if (!out) throw std::runtime_error("Cannot open file for writing: " + path);
        for (const auto& s : stream) {
            out << s << "\n";
        }
    }

    static std::vector<std::string> loadFromFile(const std::string& path) {
        std::ifstream in(path);
        if (!in) throw std::runtime_error("Cannot open file for reading: " + path);

        std::vector<std::string> stream;
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            stream.push_back(line);
        }
        return stream;
    }

private:
    Config cfg_;
    std::mt19937_64 rng_;
};