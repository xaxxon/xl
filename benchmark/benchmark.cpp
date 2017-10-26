
#include <benchmark/benchmark.h>


void register_equality_tests();



static void no_op(benchmark::State& state) {

    while (state.KeepRunning()) {
    }
}
BENCHMARK(no_op);



//BENCHMARK_MAIN();
int main(int argc, char** argv) {

//    register_equality_tests();
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}
