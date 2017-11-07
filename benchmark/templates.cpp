

#include <iostream>
#include <sstream>
#include <benchmark/benchmark.h>

#include "templates.h"

using namespace xl;
using namespace xl::templates;
using namespace std;


static void single_use_template(benchmark::State& state) {

    Provider p{std::pair("SUB1", "RESULT1"), std::pair("SUB2", "RESULT2")};
    while (state.KeepRunning()) {
        for(int i = 0; i < 1000; i++) {
            benchmark::DoNotOptimize(Template("This is some text {SUB1} this is some more text {SUB2}").fill(p));
        }
    }
}
BENCHMARK(single_use_template);


static void precompiled_template(benchmark::State& state) {
    Template t("This is some text {SUB1} this is some more text {SUB2}");
    t.compile();
    Provider p{std::pair("SUB1", "RESULT1"), std::pair("SUB2", "RESULT2")};
    while (state.KeepRunning()) {
        for(int i = 0; i < 1000; i++) {
            benchmark::DoNotOptimize(t.fill(p));
        }
    }
}
BENCHMARK(precompiled_template);
