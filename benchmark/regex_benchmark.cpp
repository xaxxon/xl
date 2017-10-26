

#include <iostream>
#include <sstream>
#include <benchmark/benchmark.h>
#include <vector>
#include <sstream>


#include "regex/regexer.h"

//using namespace xl;

static void std_regex_plain_match(benchmark::State& state) {

    std::regex regex("^([^.]*)\\.(.*)$");
    std::string source("This is a long string with a . in it");

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(std::regex_match(source, regex));
    }
}
BENCHMARK(std_regex_plain_match);


static void xl_regex_std_match(benchmark::State& state) {

    xl::RegexStd regex("^([^.]*)\\.(.*)$");
    std::string source("This is a long string with a . in it");

    // sanity check to make sure the match is happening and it is matching the right data
    assert(regex.match(source)[1] == "This is a long string with a ");
    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(regex.match(source)[0]);
    }
}
BENCHMARK(xl_regex_std_match);





static void xl_regex_pcre_match(benchmark::State& state) {
    xl::RegexPcre regex("^([^.]*)\\.(.*)$");
    std::string source("This is a long string with a . in it");

    // sanity check to make sure the match is happening and it is matching the right data
    assert(regex.match(source)[1] == "This is a long string with a ");

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(regex.match(source)[0]);
    }

}
BENCHMARK(xl_regex_pcre_match);




// Benchmark replace

static void xl_std_regex_replace(benchmark::State& state) {
    xl::RegexStd regex("^([^.]*)\\.(.*)$");
    std::string source("This is a long string with a . in it");

    // sanity check to make sure the match is happening and it is matching the right data
    assert(regex.replace(source, "$2:$1") == " in it:This is a long string with a ");

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(regex.replace(source, "$2:$1"));
    }

}
BENCHMARK(xl_std_regex_replace);



static void xl_pcre_regex_replace(benchmark::State& state) {
    xl::RegexPcre regex("^([^.]*)\\.(.*)$");
    std::string source("This is a long string with a . in it");

    // sanity check to make sure the match is happening and it is matching the right data
    assert(regex.replace(source, "$2:$1") == " in it:This is a long string with a ");

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(regex.replace(source, "$2:$1"));
    }

}
BENCHMARK(xl_pcre_regex_replace);
