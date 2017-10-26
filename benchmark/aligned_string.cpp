

#include <iostream>
#include <sstream>
#include <benchmark/benchmark.h>
#include <vector>
#include <sstream>

#include "aligned_string/aligned_string.h"
#include "aligned_string/static_buffer.h"

using namespace xl;
using namespace std;


std::string gen_random(const int len, char c = '\0') {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    std::stringstream ss;

    for (int i = 0; i < len; ++i) {
        if (c == '\0') {
            ss << alphanum[rand() % (sizeof(alphanum) - 1)];
        } else {
            ss << c;
        }
    }
    return ss.str();
}





static void aligned_short_string_strchr_miss(benchmark::State& state) {

    std::vector<AlignedString<AlignedStringBuffer_Static<16>>> strings = {"aaaaaaaaaaz", "bbbbbbbbbbz", "ccccccccccz", "ddddddddddz"};

    while (state.KeepRunning()) {
        for(auto & s : strings) {
            benchmark::DoNotOptimize(s.strchr('x'));
        }
    }
}
BENCHMARK(aligned_short_string_strchr_miss);


static void std_short_string_find_miss(benchmark::State& state) {

    std::vector<std::string> strings = {"aaaaaaaaaaz", "bbbbbbbbbbz", "ccccccccccz", "ddddddddddz"};

    while (state.KeepRunning()) {
        for(auto & s : strings) {
            benchmark::DoNotOptimize(s.find('x'));
        }
//        assert(b == std::string::npos);

    }
}
BENCHMARK(std_short_string_find_miss);


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


static void std_short_string_find_hit(benchmark::State& state) {

    std::vector<std::string> strings = {"aaaaaaaaaaz", "bbbbbbbbbbz", "ccccccccccz", "ddddddddddz"};

    while (state.KeepRunning()) {
        for(auto & s : strings) {
            benchmark::DoNotOptimize(s.find('z'));
        }
//        assert(b != std::string::npos);

    }
}
BENCHMARK(std_short_string_find_hit);


static void aligned_short_string_strchr_hit_first(benchmark::State& state) {

    std::vector<AlignedString<AlignedStringBuffer_Static<16>>> strings = {"aaaaaaaaaaz", "abbbbbbbbbz", "acccccccccz", "adddddddddz"};

    while (state.KeepRunning()) {
        for(auto & s : strings) {
            benchmark::DoNotOptimize(s.strchr('a'));
        }

    }
}
BENCHMARK(aligned_short_string_strchr_hit_first);


static void std_short_string_find_hit_first(benchmark::State& state) {

    std::vector<std::string> strings = {"aaaaaaaaaaz", "abbbbbbbbbz", "acccccccccz", "adddddddddz"};

    volatile std::string::size_type b = std::string::npos;
    while (state.KeepRunning()) {
        for(auto & s : strings) {
            benchmark::DoNotOptimize(s.find('a'));

        }
//        assert(b != std::string::npos);

    }
}
BENCHMARK(std_short_string_find_hit_first);

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



template<class StringType>
vector<StringType> make_strings(size_t count, size_t length, char contents='\0') {
    vector<StringType> strings;

    for(int i = 0; i < count; i++) {
        strings.push_back(gen_random(length, contents));
    }

    return strings;
}


static void aligned_short_string_strchr_random(benchmark::State& state) {

    auto strings = make_strings<AlignedString<AlignedStringBuffer_Static<64>>>(100, 62);


    while (state.KeepRunning()) {
        for(auto & s : strings) {
            benchmark::DoNotOptimize(s.strchr<true>('a'));
        }
    }
}
BENCHMARK(aligned_short_string_strchr_random);


static void aligned_short_string_strchr_random_expect_no_match(benchmark::State& state) {

    auto strings = make_strings<AlignedString<AlignedStringBuffer_Static<64>>>(100, 62);


    while (state.KeepRunning()) {
        for(auto & s : strings) {
            benchmark::DoNotOptimize(s.strchr<false>('a'));
        }
    }
}
BENCHMARK(aligned_short_string_strchr_random_expect_no_match);




static void std_string_strchr_random(benchmark::State& state) {

    std::vector<std::string> strings;
    for(int i = 0; i < 100; i++) {
        strings.push_back(gen_random(62));
    }

    while (state.KeepRunning()) {
        for(auto & s : strings) {
            benchmark::DoNotOptimize(strchr(s.c_str(), 'a'));
        }
    }
}
BENCHMARK(std_string_strchr_random);

static void std_string_find_random(benchmark::State& state) {

    std::vector<std::string> strings;
    for(int i = 0; i < 100; i++) {
        strings.push_back(gen_random(62));
    }

    while (state.KeepRunning()) {
        for(auto & s : strings) {
            benchmark::DoNotOptimize(s.find('a'));
        }
    }
}
BENCHMARK(std_string_find_random);




static void aligned_short_string_equals(benchmark::State& state) {

    auto strings = make_strings<AlignedString<AlignedStringBuffer_Static<16>>>(100, 14, 'a');

    while (state.KeepRunning()) {
        for(auto & s : strings) {
            benchmark::DoNotOptimize(s == strings[0]);
        }
    }
}
BENCHMARK(aligned_short_string_equals);


static void std_short_string_equals(benchmark::State& state) {

    auto strings = make_strings<std::string>(100, 14, 'a');

    while (state.KeepRunning()) {
        for(auto & s : strings) {
            benchmark::DoNotOptimize(s == strings[0]);
        }
    }
}
BENCHMARK(std_short_string_equals);


void register_equality_tests() {
    auto strings = make_strings<std::string>(100, 14, 'a');

    auto strcmp_check = [](benchmark::State& state, auto & main_string, auto & strings){
//        std::cerr << "Comparing " << main_string << " against " << strings[0] << std::endl;
        while (state.KeepRunning()) {
            for(auto & s : strings) {
                benchmark::DoNotOptimize(strcmp(s.c_str(), main_string.c_str()));
            }
        }
    };
    auto string_op_equals_check = [](benchmark::State& state, auto & main_string, auto & strings){
        while (state.KeepRunning()) {
            for(auto & s : strings) {
                benchmark::DoNotOptimize(main_string == s);
            }
        }
    };

//    for (int i = 8; i <= 128; i += 1) {
//
//        // make 100 strings of length i containing the letter 'a'
//        auto strings = make_strings<std::string>(100, i, 'a');
//        stringstream name;
//        name << "matching_string_short_string_strcmp" << i;
//        benchmark::RegisterBenchmark(name.str().c_str(), strcmp_check, std::string(strings[0]),
//                                     strings);
//    }

    auto short_string = "aaaaaaaaaaaaaa";
    benchmark::RegisterBenchmark("matching_string_std_string_op_eq", string_op_equals_check,  std::string(short_string), make_strings<std::string>(100, 14, 'a'));
    benchmark::RegisterBenchmark("matching_string_aligned_string_16_op_eq", string_op_equals_check,  AlignedString<AlignedStringBuffer_Static<16>>(short_string), make_strings<AlignedString<AlignedStringBuffer_Static<16>>>(100, 14, 'a'));

    benchmark::RegisterBenchmark("early_mismatch_string_short_stream_strcmp", strcmp_check, std::string(short_string), make_strings<std::string>(100, 14, 'a'));
    benchmark::RegisterBenchmark("early_mismatch_string_std_string_op_eq", string_op_equals_check,  std::string(short_string), make_strings<std::string>(100, 14, 'a'));
    benchmark::RegisterBenchmark("early_mismatch_string_aligned_string_16_op_eq", string_op_equals_check,  AlignedString<AlignedStringBuffer_Static<16>>(short_string), make_strings<AlignedString<AlignedStringBuffer_Static<16>>>(100, 14, 'a'));

    benchmark::RegisterBenchmark("late_mismatch_string_short_stream_strcmp", strcmp_check, std::string(short_string), make_strings<std::string>(100, 14, 'a'));
    benchmark::RegisterBenchmark("late_mismatch_string_std_string_op_eq", string_op_equals_check,  std::string(short_string), make_strings<std::string>(100, 14, 'a'));
    benchmark::RegisterBenchmark("late_mismatch_string_aligned_string_16_op_eq", string_op_equals_check,  AlignedString<AlignedStringBuffer_Static<16>>(short_string), make_strings<AlignedString<AlignedStringBuffer_Static<16>>>(100, 14, 'a'));

    auto long_string = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    assert(strlen(long_string) == 62);
    benchmark::RegisterBenchmark("matching_string_long_string_strcmp", strcmp_check, std::string(long_string), make_strings<std::string>(100, 62, 'a'));
    benchmark::RegisterBenchmark("matching_string_std_string_op_eq", string_op_equals_check,  std::string(long_string), make_strings<std::string>(100, 62, 'a'));
    benchmark::RegisterBenchmark("matching_string_aligned_string_64_op_eq", string_op_equals_check,  AlignedString<AlignedStringBuffer_Static<64>>(long_string), make_strings<AlignedString<AlignedStringBuffer_Static<64>>>(100, 62, 'a'));

    benchmark::RegisterBenchmark("early_mismatch_string_long_stream_strcmp", strcmp_check, std::string(long_string), make_strings<std::string>(100, 62, 'a'));
    benchmark::RegisterBenchmark("early_mismatch_string_std_string_op_eq", string_op_equals_check,  std::string(long_string), make_strings<std::string>(100, 62, 'a'));
    benchmark::RegisterBenchmark("early_mismatch_string_aligned_string_64_op_eq", string_op_equals_check,  AlignedString<AlignedStringBuffer_Static<64>>(long_string), make_strings<AlignedString<AlignedStringBuffer_Static<64>>>(100, 62, 'a'));

    benchmark::RegisterBenchmark("late_mismatch_string_long_stream_strcmp", strcmp_check, std::string(long_string), make_strings<std::string>(100, 62, 'a'));
    benchmark::RegisterBenchmark("late_mismatch_string_std_string_op_eq", string_op_equals_check,  std::string(long_string), make_strings<std::string>(100, 62, 'a'));
    benchmark::RegisterBenchmark("late_mismatch_string_aligned_string_64_op_eq", string_op_equals_check,  AlignedString<AlignedStringBuffer_Static<64>>(long_string), make_strings<AlignedString<AlignedStringBuffer_Static<64>>>(100, 62, 'a'));

}


