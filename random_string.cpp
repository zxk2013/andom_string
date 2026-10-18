#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <exception>
#include <iostream>
#include <random>
#include <string>
#include <utility>

// Returns a seed without ever throwing. Falls back to the clock if
// std::random_device construction or sampling throws on this platform.
static unsigned int makeSeed() noexcept {
    try {
        std::random_device rd;
        return rd();
    } catch (...) {
        const auto now = std::chrono::high_resolution_clock::now()
                             .time_since_epoch()
                             .count();
        return static_cast<unsigned int>(now);
    }
}

class PlaceholderGenerator {
public:
    explicit PlaceholderGenerator(std::string prefix = "-",
                                  std::size_t suffixLength = 32,
                                  unsigned int seed = makeSeed())
        : prefix_(std::move(prefix)),
          suffixLength_(suffixLength),
          rng_(seed) {}

    std::string generate() {
        static constexpr char kHex[] = "0123456789abcdef";
        static constexpr std::size_t kBlockCount = 256;
        const std::size_t pairCount = suffixLength_ / 2;
        const std::size_t distinctCount = std::min(pairCount, kBlockCount);

        std::array<std::size_t, kBlockCount> idx{};
        for (std::size_t i = 0; i < kBlockCount; ++i) idx[i] = i;

        std::string suffix;
        suffix.reserve(suffixLength_);

        for (std::size_t i = 0; i < distinctCount; ++i) {
            std::uniform_int_distribution<std::size_t> dist(i, kBlockCount - 1);
            std::swap(idx[i], idx[dist(rng_)]);
            const std::size_t v = idx[i];
            suffix.push_back(kHex[v >> 4]);
            suffix.push_back(kHex[v & 0x0F]);
        }

        std::uniform_int_distribution<std::size_t> hexDist(0, 15);
        for (std::size_t i = distinctCount; i < pairCount; ++i) {
            suffix.push_back(kHex[hexDist(rng_)]);
            suffix.push_back(kHex[hexDist(rng_)]);
        }

        // Fix: emit the final character for odd lengths instead of dropping one.
        if (suffixLength_ % 2 != 0) {
            suffix.push_back(kHex[hexDist(rng_)]);
        }

        return prefix_ + suffix;
    }

    void setPrefix(std::string prefix) { prefix_ = std::move(prefix); }
    void setSuffixLength(std::size_t length) { suffixLength_ = length; }

    const std::string& prefix() const noexcept { return prefix_; }
    std::size_t suffixLength() const noexcept { return suffixLength_; }

private:
    std::string prefix_;
    std::size_t suffixLength_;
    std::mt19937_64 rng_;
};

static bool parseArgs(int argc, char** argv,
                      std::string& prefix, std::size_t& len, int& count) {
    static constexpr std::size_t kMaxSuffixLength = 1'000'000;
    try {
        if (argc > 1) prefix = argv[1];
        if (argc > 2) {
            long long v = std::stoll(argv[2]);
            if (v < 0 || static_cast<unsigned long long>(v) > kMaxSuffixLength)
                return false;
            len = static_cast<std::size_t>(v);
        }
        if (argc > 3) {
            long long c = std::stoll(argv[3]);
            if (c < 0 || c > 1'000'000) return false;
            count = static_cast<int>(c);
        }
    } catch (...) {
        return false;
    }
    return true;
}

int main(int argc, char** argv) {
    try {
        std::string prefix = "-";
        std::size_t length = 32;
        int count = 5;

        if (!parseArgs(argc, argv, prefix, length, count)) {
            std::cerr << "Usage: " << argv[0]
                      << " [prefix] [suffix_length] [count]\n"
                      << "  prefix         default: -\n"
                      << "  suffix_length  default: 32 (max 1000000)\n"
                      << "  count          default: 5  (max 1000000)\n";
            return 1;
        }

        PlaceholderGenerator gen(prefix, length);
        for (int i = 0; i < count; ++i) {
            std::cout << gen.generate() << '\n';
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "Error: unknown exception\n";
        return 1;
    }
}