#include <iostream>
#include "RandomStreamGen.cpp"

int main() {
    RandomStreamGen::Config cfg;
    cfg.seed = 42;
    RandomStreamGen gen(cfg);
    auto stream = gen.generate(100000);

    auto steps = RandomStreamGen::prefixSizesByPercent(stream.size(), 10);

    std::cout << "Generated: " << stream.size() << " strings\n";
    std::cout << "Steps count: " << steps.size() << "\n";
    std::cout << "First step prefix size: " << steps.front() << "\n";
    std::cout << "Last step prefix size: " << steps.back() << "\n";
    std::cout << "Example string: " << stream[0] << " len=" << stream[0].size() << "\n";
}