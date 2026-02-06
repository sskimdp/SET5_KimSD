#pragma once
#include <cstdint>
#include <string>
#include <random>
#include <vector>
#include <stdexcept>

class HashFunc {
public:
    explicit HashFunc(std::uint64_t seed) : seed_(seed) {}

    std::uint32_t operator()(const std::string& s) const {
        std::uint64_t h = 14695981039346656037ULL ^ seed_;
        for (unsigned char c : s) {
            h ^= static_cast<std::uint64_t>(c);
            h *= 1099511628211ULL;
        }
        h = mix64(h);
        return static_cast<std::uint32_t>(h & 0xFFFFFFFFULL);
    }

private:
    std::uint64_t seed_;

    static std::uint64_t mix64(std::uint64_t x) {
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        x = x ^ (x >> 31);
        return x;
    }
};

class HashFuncGen {
public:
    explicit HashFuncGen(std::uint64_t masterSeed = 123456789ULL)
        : rng_(masterSeed) {}

    HashFunc make() {
        std::uint64_t seed = dist_(rng_);
        return HashFunc(seed);
    }

    std::vector<HashFunc> makeMany(std::size_t k) {
        std::vector<HashFunc> funcs;
        funcs.reserve(k);
        for (std::size_t i = 0; i < k; ++i) funcs.push_back(make());
        return funcs;
    }

private:
    std::mt19937_64 rng_;
    std::uniform_int_distribution<std::uint64_t> dist_;
};