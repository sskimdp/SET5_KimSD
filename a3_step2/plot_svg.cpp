#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <limits>
#include <cctype>

static inline std::string trim(const std::string& s) {
    std::size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) a++;
    while (b > a && std::isspace((unsigned char)s[b - 1])) b--;
    return s.substr(a, b - a);
}

static inline std::vector<std::string> split(const std::string& line, char delim) {
    std::vector<std::string> out;
    std::string cur;
    std::istringstream ss(line);
    while (std::getline(ss, cur, delim)) out.push_back(trim(cur));
    return out;
}

static inline double parseDouble(std::string s) {
    s = trim(s);
    for (char& c : s) if (c == ',') c = '.';
    return std::stod(s);
}

static inline int parseInt(std::string s) {
    s = trim(s);
    return std::stoi(s);
}

struct ExRow {
    int step;
    double processed;
    double f0;
    double n;
};

struct StRow {
    int step;
    double processed;
    double mean;
    double stdev;
};

struct SvgCanvas {
    int W, H;
    int left = 80, right = 30, top = 30, bottom = 60;

    int plotW() const { return W - left - right; }
    int plotH() const { return H - top - bottom; }
};

static std::string fmt(double v, int prec = 0) {
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss.precision(prec);
    oss << v;
    return oss.str();
}

static void writeSvgHeader(std::ofstream& out, int W, int H) {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << W
        << "\" height=\"" << H << "\" viewBox=\"0 0 " << W << " " << H << "\">\n";
    out << "<rect x=\"0\" y=\"0\" width=\"" << W << "\" height=\"" << H
        << "\" fill=\"white\" />\n";
    out << "<style>\n"
        << "  .axis { stroke:#000; stroke-width:1; }\n"
        << "  .grid { stroke:#ddd; stroke-width:1; }\n"
        << "  .label { font-family: Arial, sans-serif; font-size: 12px; fill:#000; }\n"
        << "  .title { font-family: Arial, sans-serif; font-size: 14px; font-weight:bold; fill:#000; }\n"
        << "</style>\n";
}

static void writeSvgFooter(std::ofstream& out) {
    out << "</svg>\n";
}

static double mapX(double x, double xmin, double xmax, const SvgCanvas& c) {
    if (xmax == xmin) return c.left;
    return c.left + (x - xmin) / (xmax - xmin) * c.plotW();
}

static double mapY(double y, double ymin, double ymax, const SvgCanvas& c) {
    if (ymax == ymin) return c.top + c.plotH();
    return c.top + c.plotH() - (y - ymin) / (ymax - ymin) * c.plotH();
}

static void drawAxesAndGrid(std::ofstream& out, const SvgCanvas& c,
                            double xmin, double xmax, double ymin, double ymax,
                            const std::string& xLabel, const std::string& yLabel,
                            const std::string& title,
                            int xTicks = 10) {

    out << "<text class=\"title\" x=\"" << (c.W/2) << "\" y=\"18\" text-anchor=\"middle\">"
        << title << "</text>\n";

    for (int i = 0; i <= xTicks; ++i) {
        double tx = xmin + (xmax - xmin) * i / xTicks;
        double px = mapX(tx, xmin, xmax, c);

        out << "<line class=\"grid\" x1=\"" << px << "\" y1=\"" << c.top
            << "\" x2=\"" << px << "\" y2=\"" << (c.top + c.plotH()) << "\" />\n";

        out << "<text class=\"label\" x=\"" << px << "\" y=\"" << (c.top + c.plotH() + 20)
            << "\" text-anchor=\"middle\">" << fmt(tx, 0) << "</text>\n";
    }

    const int yTicks = 5;
    for (int i = 0; i <= yTicks; ++i) {
        double ty = ymin + (ymax - ymin) * i / yTicks;
        double py = mapY(ty, ymin, ymax, c);

        out << "<line class=\"grid\" x1=\"" << c.left << "\" y1=\"" << py
            << "\" x2=\"" << (c.left + c.plotW()) << "\" y2=\"" << py << "\" />\n";

        out << "<text class=\"label\" x=\"" << (c.left - 8) << "\" y=\"" << (py + 4)
            << "\" text-anchor=\"end\">" << fmt(ty, 0) << "</text>\n";
    }

    out << "<line class=\"axis\" x1=\"" << c.left << "\" y1=\"" << (c.top + c.plotH())
        << "\" x2=\"" << (c.left + c.plotW()) << "\" y2=\"" << (c.top + c.plotH()) << "\" />\n";
    out << "<line class=\"axis\" x1=\"" << c.left << "\" y1=\"" << c.top
        << "\" x2=\"" << c.left << "\" y2=\"" << (c.top + c.plotH()) << "\" />\n";

    out << "<text class=\"label\" x=\"" << (c.W/2) << "\" y=\"" << (c.H - 15)
        << "\" text-anchor=\"middle\">" << xLabel << "</text>\n";

    out << "<text class=\"label\" x=\"18\" y=\"" << (c.H/2)
        << "\" text-anchor=\"middle\" transform=\"rotate(-90 18 " << (c.H/2) << ")\">"
        << yLabel << "</text>\n";
}

static void drawPolyline(std::ofstream& out, const std::vector<std::pair<double,double>>& pts,
                         double xmin, double xmax, double ymin, double ymax,
                         const SvgCanvas& c, const std::string& stroke, int width = 2,
                         const std::string& dashArray = "") {
    out << "<polyline fill=\"none\" stroke=\"" << stroke << "\" stroke-width=\"" << width << "\"";
    if (!dashArray.empty()) out << " stroke-dasharray=\"" << dashArray << "\"";
    out << " points=\"";
    for (auto [x,y] : pts) {
        out << mapX(x, xmin, xmax, c) << "," << mapY(y, ymin, ymax, c) << " ";
    }
    out << "\" />\n";
}

static void drawLegend(std::ofstream& out, const SvgCanvas& c,
                       const std::vector<std::pair<std::string,std::string>>& items) {
    int x0 = c.left + 10;
    int y0 = c.top + 10;
    int dy = 18;

    for (int i = 0; i < (int)items.size(); ++i) {
        int y = y0 + i*dy;
        out << "<line x1=\"" << x0 << "\" y1=\"" << y << "\" x2=\"" << (x0+20) << "\" y2=\"" << y
            << "\" stroke=\"" << items[i].second << "\" stroke-width=\"3\" />\n";
        out << "<text class=\"label\" x=\"" << (x0+28) << "\" y=\"" << (y+4)
            << "\" text-anchor=\"start\">" << items[i].first << "</text>\n";
    }
}

static void drawBand(std::ofstream& out,
                     const std::vector<std::pair<double,double>>& upper,
                     const std::vector<std::pair<double,double>>& lower,
                     double xmin, double xmax, double ymin, double ymax,
                     const SvgCanvas& c,
                     const std::string& fill, double opacity = 0.25) {
    out << "<polygon fill=\"" << fill << "\" fill-opacity=\"" << opacity
        << "\" stroke=\"none\" points=\"";
    for (auto [x,y] : upper) {
        out << mapX(x, xmin, xmax, c) << "," << mapY(y, ymin, ymax, c) << " ";
    }
    for (int i = (int)lower.size() - 1; i >= 0; --i) {
        auto [x,y] = lower[i];
        out << mapX(x, xmin, xmax, c) << "," << mapY(y, ymin, ymax, c) << " ";
    }
    out << "\" />\n";
}

static std::vector<ExRow> readExample(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Cannot open: " + path);

    std::vector<ExRow> rows;
    std::string line;
    std::getline(in, line);

    while (std::getline(in, line)) {
        if (trim(line).empty()) continue;
        auto parts = split(line, ';');
        if (parts.size() < 4) continue;

        ExRow r;
        r.step      = parseInt(parts[0]);
        r.processed = parseDouble(parts[1]);
        r.f0        = parseDouble(parts[2]);
        r.n         = parseDouble(parts[3]);
        rows.push_back(r);
    }
    return rows;
}

static std::vector<StRow> readStats(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Cannot open: " + path);

    std::vector<StRow> rows;
    std::string line;
    std::getline(in, line);

    while (std::getline(in, line)) {
        if (trim(line).empty()) continue;
        auto parts = split(line, ';');
        if (parts.size() < 4) continue;

        StRow r;
        r.step      = parseInt(parts[0]);
        r.processed = parseDouble(parts[1]);
        r.mean      = parseDouble(parts[2]);
        r.stdev     = parseDouble(parts[3]);
        rows.push_back(r);
    }
    return rows;
}

static void plotGraph1Svg(const std::vector<ExRow>& ex, const std::string& outPath) {
    SvgCanvas c{900, 520};

    double xmin = ex.front().step;
    double xmax = ex.back().step;

    double ymin = 0.0;
    double ymax = 0.0;
    for (const auto& r : ex) ymax = std::max({ymax, r.f0, r.n});
    ymax *= 1.05;

    std::ofstream out(outPath);
    writeSvgHeader(out, c.W, c.H);

    drawAxesAndGrid(out, c, xmin, xmax, ymin, ymax,
                    "step (t)", "unique elements",
                    "Graph 1: True F0^t vs HLL estimate N_t",
                    (int)(xmax - xmin));

    std::vector<std::pair<double,double>> f0pts, npts;
    for (const auto& r : ex) {
        f0pts.push_back({(double)r.step, r.f0});
        npts.push_back({(double)r.step, r.n});
    }

    drawPolyline(out, f0pts, xmin, xmax, ymin, ymax, c, "#000000", 3);
    drawPolyline(out, npts,  xmin, xmax, ymin, ymax, c, "#1f77b4", 3);

    drawLegend(out, c, {
        {"F0^t (true)", "#000000"},
        {"N_t (HLL)",   "#1f77b4"}
    });

    writeSvgFooter(out);
}

static void plotGraph2Svg(const std::vector<StRow>& st, const std::string& outPath) {
    SvgCanvas c{900, 520};

    double xmin = st.front().step;
    double xmax = st.back().step;

    double ymin = 0.0;
    double ymax = 0.0;
    for (const auto& r : st) ymax = std::max(ymax, r.mean + r.stdev);
    ymax *= 1.05;

    std::ofstream out(outPath);
    writeSvgHeader(out, c.W, c.H);

    drawAxesAndGrid(out, c, xmin, xmax, ymin, ymax,
                    "step (t)", "estimated unique elements",
                    "Graph 2: E(N_t) with uncertainty band (±σ)",
                    (int)(xmax - xmin));

    std::vector<std::pair<double,double>> meanPts, upperPts, lowerPts;
    for (const auto& r : st) {
        meanPts.push_back({(double)r.step, r.mean});
        upperPts.push_back({(double)r.step, r.mean + r.stdev});
        lowerPts.push_back({(double)r.step, std::max(0.0, r.mean - r.stdev)});
    }

    drawBand(out, upperPts, lowerPts, xmin, xmax, ymin, ymax, c, "#240ea4", 5);
    drawPolyline(out, meanPts, xmin, xmax, ymin, ymax, c, "#74b6e5", 3);
    drawPolyline(out, upperPts, xmin, xmax, ymin, ymax, c, "#d62728", 2, "5 3");
    drawPolyline(out, lowerPts, xmin, xmax, ymin, ymax, c, "#000000", 2, "5 3");

    drawLegend(out, c, {
        {"E(N_t)", "#74b6e5"},
        {"E(N_t)+σ", "#d62728"},
        {"E(N_t)-σ", "#000000"},
        {"band: between ±σ", "#240ea4"}
    });

    writeSvgFooter(out);
}

int main() {
    try {
        auto ex = readExample("stage2_example.csv");
        auto st = readStats("stage2_stats.csv");

        if (ex.empty() || st.empty()) {
            std::cerr << "Empty data. Check CSV files.\n";
            return 1;
        }

        plotGraph1Svg(ex, "graph1.svg");
        plotGraph2Svg(st, "graph2.svg");

        std::cout << "Saved: graph1.svg and graph2.svg\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}