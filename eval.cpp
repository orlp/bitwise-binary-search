#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <cstdint>

#include "skarupke_binary_search.h"
#include "bitwise_binary_search.h"

template<typename It, typename T, typename Cmp>
It lower_bound_linear(It begin, It end, const T& value, Cmp comp) {
    size_t n = end - begin;
    size_t i = 0;
    while (i < n && comp(begin[i], value)) i += 1;
    return begin + i;
}

std::vector<uint32_t> sorted_int(size_t n) {
    std::vector<uint32_t> v; v.reserve(n);
    for (uint32_t i = 0; i < n; ++i) v.push_back(i);
    return v;
}

template<class T>
std::vector<std::string> to_strings(const std::vector<T>& v, size_t width) {
    std::vector<std::string> vs; vs.reserve(v.size());
    for (auto i : v) {
        std::ostringstream ss;
        ss << std::setw(width) << std::setfill('0') << i << "\n";
        vs.push_back(ss.str());
    }
    return vs;
}

std::vector<size_t> rand_ints(size_t n, size_t min, size_t max, std::mt19937_64& rng) {
    std::uniform_int_distribution<> distrib(min, max);
    std::vector<size_t> v; v.reserve(n);
    for (size_t i = 0; i < n; ++i) v.push_back(distrib(rng));
    return v;
}


struct CountCmp {
    size_t& num;

    CountCmp(size_t& num) : num(num) { }

    template<class T>
    bool operator()(const T& lhs, const T& rhs) const {
        this->num += 1;
        return lhs < rhs;
    }
};

typedef const uint32_t* (*IntBoundF)(const uint32_t*, const uint32_t*, const uint32_t&, std::less<uint32_t>);
typedef const uint32_t* (*CountBoundF)(const uint32_t*, const uint32_t*, const uint32_t&, CountCmp);
typedef const std::string* (*StrBoundF)(const std::string*, const std::string*, const std::string&, std::less<std::string>);

std::tuple<std::string, IntBoundF, CountBoundF, StrBoundF> algos[] = {
    {
        "lower_bound_linear",
        lower_bound_linear<const uint32_t*, uint32_t, std::less<uint32_t>>,
        lower_bound_linear<const uint32_t*, uint32_t, CountCmp>,
        lower_bound_linear<const std::string*, std::string, std::less<std::string>>
    },

    {
        "lower_bound_std",
        std::lower_bound<const uint32_t*, uint32_t, std::less<uint32_t>>,
        std::lower_bound<const uint32_t*, uint32_t, CountCmp>,
        std::lower_bound<const std::string*, std::string, std::less<std::string>>
    },

    {
        "lower_bound_pad",
        lower_bound_pad<const uint32_t*, uint32_t, std::less<uint32_t>>,
        lower_bound_pad<const uint32_t*, uint32_t, CountCmp>,
        lower_bound_pad<const std::string*, std::string, std::less<std::string>>
    },

    {
        "lower_bound_overlap",
        lower_bound_overlap<const uint32_t*, uint32_t, std::less<uint32_t>>,
        lower_bound_overlap<const uint32_t*, uint32_t, CountCmp>,
        lower_bound_overlap<const std::string*, std::string, std::less<std::string>>
    },

    {
        "lower_bound_opt",
        lower_bound_opt<const uint32_t*, uint32_t, std::less<uint32_t>>,
        lower_bound_opt<const uint32_t*, uint32_t, CountCmp>,
        lower_bound_opt<const std::string*, std::string, std::less<std::string>>
    },
    
    {
        "lower_bound_skarupke",
        branchless_lower_bound<const uint32_t*, uint32_t, std::less<uint32_t>>,
        branchless_lower_bound<const uint32_t*, uint32_t, CountCmp>,
        branchless_lower_bound<const std::string*, std::string, std::less<std::string>>
    },
};



int main(int argc, char** argv) {
    std::mt19937_64 rng;

    // Correctness and comparison count.
    std::ofstream comp_count_out("comp-counts.csv");
    comp_count_out << "algo,n,num_cmp\n";
    for (auto [algo, bound_int, bound_cmp, bound_str] : algos) {
        for (size_t n = 0; n <= 256; ++n) {
            std::vector<uint32_t> arr = sorted_int(n);

            size_t num_cmp = 0;
            CountCmp cmp(num_cmp);

            for (uint32_t rank = 0; rank <= n; ++rank) {
                size_t ret = bound_cmp(arr.data(), arr.data() + n, rank, cmp) - arr.data();
                if (ret != rank) {
                    std::cerr << "Algorithm " << algo << " returned incorrect result on size " << n << ".\n";
                    return 1;
                }
            }

            comp_count_out << algo << "," << n << "," << double(num_cmp) / double(n + 1) << std::endl;
        }
    }
    comp_count_out.close();

    std::vector<size_t> sizes;
    for (size_t n = 0; n < 256; ++n) {
        sizes.push_back(n);
    }
    double scale = std::pow(2.0, 1.0/12.0);
    for (double nf = 256.0; nf <= double(1 << 20) + 0.45; nf *= scale) {
        sizes.push_back(size_t(nf + 0.5));
    }

    // Speed.
    const size_t repeats = 1000000;
    std::ofstream speed_out("speed.csv");
    speed_out << "algo,n,nanosec,dummy\n";
    for (auto [algo, bound_int, bound_cmp, bound_str] : algos) {
        rng.seed(0xdeadbeef);

        for (auto n : sizes) {
            if (algo == "lower_bound_linear" and n > 256) continue;
            std::cout << "Benchmarking " << algo << " " << n << " (int).\n";
            std::vector<uint32_t> arr = sorted_int(n);
            std::vector<size_t> ranks = rand_ints(repeats, 0, n, rng);

            size_t dummy = 0; // Prevents optimizing out.
            auto start = std::chrono::high_resolution_clock::now();
            for (auto rank : ranks) {
                size_t ret = bound_int(arr.data(), arr.data() + n, rank, std::less<uint32_t>()) - arr.data();
                dummy += ret;
            }
            auto stop = std::chrono::high_resolution_clock::now();

            uint64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
            speed_out << algo << "," << n << "," << double(ns) / double(repeats) << "," << dummy << std::endl;
        }
    }
    speed_out.close();


    // Speed for strings.
    const size_t str_repeats = 300000;
    const size_t str_width = 4;
    std::ofstream speed_str_out("speed-str.csv");
    speed_str_out << "algo,n,nanosec,dummy\n";
    for (auto [algo, bound_int, bound_cmp, bound_str] : algos) {
        rng.seed(0xdeadbeef);

        for (auto n : sizes) {
            if (algo == "lower_bound_linear" and n > 256) continue;
            std::cout << "Benchmarking " << algo << " " << n << " (str).\n";
            std::vector<std::string> arr = to_strings(sorted_int(n), str_width);
            std::vector<std::string> ranks = to_strings(rand_ints(str_repeats, 0, n, rng), str_width);

            size_t dummy = 0; // Prevents optimizing out.
            auto start = std::chrono::high_resolution_clock::now();
            for (auto rank : ranks) {
                size_t ret = bound_str(arr.data(), arr.data() + n, rank, std::less<std::string>()) - arr.data();
                dummy += ret;
            }
            auto stop = std::chrono::high_resolution_clock::now();

            uint64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
            speed_str_out << algo << "," << n << "," << double(ns) / double(str_repeats) << "," << dummy << std::endl;
        }
    }
    speed_str_out.close();

    return 0;
}
