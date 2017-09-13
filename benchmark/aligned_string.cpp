

#include <iostream>
#include <benchmark/benchmark.h>

#include "aligned_string/aligned_string.h"
#include "aligned_string/static_buffer.h"

using namespace xl;

static void control(benchmark::State& state) {

    while (state.KeepRunning()) {
    }
}

BENCHMARK(control);


static void aligned_short_string_strchr_miss(benchmark::State& state) {

    std::vector<AlignedString<AlignedStringBuffer_Static<16>>> strings = {"aaaaaaaaaaz", "bbbbbbbbbbz", "ccccccccccz", "ddddddddddz"};

    while (state.KeepRunning()) {
        for(auto & s : strings) {
            benchmark::DoNotOptimize(s.strchr('x'));
        }
    }
}
BENCHMARK(aligned_short_string_strchr_miss);


static void std_short_string_strchr_miss(benchmark::State& state) {

    std::vector<std::string> strings = {"aaaaaaaaaaz", "bbbbbbbbbbz", "ccccccccccz", "ddddddddddz"};

    while (state.KeepRunning()) {
        for(auto & s : strings) {
            benchmark::DoNotOptimize(s.find('x'));
        }
//        assert(b == std::string::npos);

    }
}
BENCHMARK(std_short_string_strchr_miss);


static void aligned_short_string_strchr_hit(benchmark::State& state) {

    std::vector<AlignedString<AlignedStringBuffer_Static<16>>> strings = {"aaaaaaaaaaz", "bbbbbbbbbbz", "ccccccccccz", "ddddddddddz"};

    while (state.KeepRunning()) {
        for(auto & s : strings) {
            benchmark::DoNotOptimize(s.strchr('z'));
        }
//        assert(b != nullptr);

    }
}
BENCHMARK(aligned_short_string_strchr_hit);


static void std_short_string_strchr_hit(benchmark::State& state) {

    std::vector<std::string> strings = {"aaaaaaaaaaz", "bbbbbbbbbbz", "ccccccccccz", "ddddddddddz"};

    while (state.KeepRunning()) {
        for(auto & s : strings) {
            benchmark::DoNotOptimize(s.find('z'));
        }
//        assert(b != std::string::npos);

    }
}
BENCHMARK(std_short_string_strchr_hit);


static void aligned_short_string_strchr_hit_first(benchmark::State& state) {

    std::vector<AlignedString<AlignedStringBuffer_Static<16>>> strings = {"aaaaaaaaaaz", "abbbbbbbbbz", "acccccccccz", "adddddddddz"};

    while (state.KeepRunning()) {
        for(auto & s : strings) {
            benchmark::DoNotOptimize(s.strchr('a'));
        }

    }
}
BENCHMARK(aligned_short_string_strchr_hit_first);


static void std_short_string_strchr_hit_first(benchmark::State& state) {

    std::vector<std::string> strings = {"aaaaaaaaaaz", "abbbbbbbbbz", "acccccccccz", "adddddddddz"};

    volatile std::string::size_type b = std::string::npos;
    while (state.KeepRunning()) {
        for(auto & s : strings) {
            benchmark::DoNotOptimize(s.find('a'));

        }
//        assert(b != std::string::npos);

    }
}
BENCHMARK(std_short_string_strchr_hit_first);

static void std_short_string_raw_strchr_hit_first(benchmark::State& state) {

    std::vector<std::string> strings = {"aaaaaaaaaaz", "abbbbbbbbbz", "acccccccccz", "adddddddddz"};

    volatile char const * b = nullptr;
    while (state.KeepRunning()) {
        for(auto & s : strings) {
            char c = 'a';
            benchmark::DoNotOptimize(strchr(s.c_str(), c++));
        }


    }
}
BENCHMARK(std_short_string_raw_strchr_hit_first);






static void aligned_short_string_equals(benchmark::State& state) {

    std::vector<AlignedString<AlignedStringBuffer_Static<16>>> strings = {"aaaaaaaaaaz", "bbbbbbbbbbz", "ccccccccccz", "ddddddddddz"};

    AlignedString<AlignedStringBuffer_Static<16>> other("aaaaaaaaaax");
    while (state.KeepRunning()) {
        for(auto & s : strings) {
            benchmark::DoNotOptimize(s == other);
        }
    }
}
BENCHMARK(aligned_short_string_equals);


static void std_short_string_equals(benchmark::State& state) {

    std::vector<std::string> strings = {"aaaaaaaaaaz", "bbbbbbbbbbz", "ccccccccccz", "ddddddddddz"};

    while (state.KeepRunning()) {
        for(auto & s : strings) {
//            b = s == "aaaaaaaaaax";
           benchmark::DoNotOptimize(strcmp(s.c_str(), "aaaaaaaaaax"));
        }
//        assert(b == std::string::npos);

    }
}
BENCHMARK(std_short_string_equals);


BENCHMARK_MAIN();
