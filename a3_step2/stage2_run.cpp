#include <iostream>
#include <vector>
#include <string>
#include <unordered_set>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <sstream>

#include "RandomStreamGen.cpp"
#include "HashFuncGen.cpp"
#include "HyperLogLog.cpp"

struct StepResult {
    std::size_t processed = 0;
    std::size_t F0 = 0;
    double Nt = 0.0;
};

static std::string excelDouble(double x, int precision = 6) {
    std::ostringstream oss;
    oss.imbue(std::locale::classic());
    oss << std::fixed << std::setprecision(precision) << x;

    std::string s = oss.str();
    for (char& c : s) {
        if (c == '.') c = ',';
    }

    while (s.size() > 1 && s.back() == '0') s.pop_back();
    if (!s.empty() && s.back() == ',') s.pop_back();

    return s;
}

static double mean(const std::vector<double>& a) {
    double s = 0.0;
    for (double x : a) s += x;
    return s / a.size();
}

static double sampleStd(const std::vector<double>& a) {
    if (a.size() < 2) return 0.0;
    double m = mean(a);
    double ss = 0.0;
    for (double x : a) {
        double d = x - m;
        ss += d * d;
    }
    return std::sqrt(ss / (a.size() - 1));
}

static std::vector<StepResult> processOneStream(
    const std::vector<std::string>& stream,
    const HashFunc& h,
    int B,
    const std::vector<std::size_t>& steps
) {
    std::unordered_set<std::string> uniq;
    uniq.reserve(stream.size());

    HyperLogLog hll(B);

    std::vector<StepResult> out;
    out.reserve(steps.size());

    std::size_t stepIdx = 0;
    std::size_t nextStop = steps[stepIdx];

    for (std::size_t i = 0; i < stream.size(); ++i) {
        const auto& s = stream[i];

        uniq.insert(s);

        hll.addHash(h(s));

        std::size_t processed = i + 1;
        if (processed == nextStop) {
            StepResult r;
            r.processed = processed;
            r.F0 = uniq.size();
            r.Nt = hll.estimate();
            out.push_back(r);

            stepIdx++;
            if (stepIdx >= steps.size()) break;
            nextStop = steps[stepIdx];
        }
    }
    return out;
}

int main() {
    const std::size_t N = 200000;
    const std::size_t K = 20;
    const std::size_t stepPercent = 10;
    const int B = 14;

    const std::uint64_t baseStreamSeed = 42;
    const std::uint64_t baseHashSeed = 777;

    auto steps = RandomStreamGen::prefixSizesByPercent(N, stepPercent);

    std::vector<std::vector<double>> Nt_values(steps.size());
    for (auto& v : Nt_values) v.reserve(K);

    std::vector<StepResult> example;

    for (std::size_t i = 0; i < K; ++i) {
        RandomStreamGen::Config cfg;
        cfg.seed = baseStreamSeed + i;
        RandomStreamGen gen(cfg);
        auto stream = gen.generate(N);

        HashFuncGen hgen(baseHashSeed + i);
        auto h = hgen.make();

        auto res = processOneStream(stream, h, B, steps);

        if (i == 0) example = res;

        for (std::size_t t = 0; t < res.size(); ++t) {
            Nt_values[t].push_back(res[t].Nt);
        }

        std::cout << "Processed stream " << (i + 1) << "/" << K << "\n";
    }

    {
        std::ofstream out("stage2_example.csv");
        out << "step;processed;F0_true;N_est\n";
        for (std::size_t t = 0; t < example.size(); ++t) {
            out << (t + 1) << ";"
                << example[t].processed << ";"
                << example[t].F0 << ";"
                << excelDouble(example[t].Nt, 3) << "\n";
        }
    }

    {
        std::ofstream out("stage2_stats.csv");
        out << "step;processed;mean_est;std_est\n";
        for (std::size_t t = 0; t < steps.size(); ++t) {
            double m = mean(Nt_values[t]);
            double s = sampleStd(Nt_values[t]);
            out << (t + 1) << ";"
                << steps[t] << ";"
                << excelDouble(m, 3) << ";"
                << excelDouble(s, 3) << "\n";
        }
    }

    std::cout << "Saved: stage2_example.csv, stage2_stats.csv\n";
    std::cout << "Params: N=" << N << ", K=" << K
              << ", step=" << stepPercent << "%, B=" << B
              << ", m=" << (1u << B) << "\n";

    return 0;
}