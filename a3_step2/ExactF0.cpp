#pragma once
#include <unordered_set>
#include <vector>
#include <string>
#include <cstddef>

inline std::size_t exactF0Prefix(const std::vector<std::string>& stream, std::size_t prefixLen) {
    std::unordered_set<std::string> uniq;
    uniq.reserve(prefixLen);
    for (std::size_t i = 0; i < prefixLen; ++i) {
        uniq.insert(stream[i]);
    }
    return uniq.size();
}