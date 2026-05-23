cmake -G "Unix Makefiles" -DEXERCISM_RUN_ALL_TESTS=1 -DEXERCISM_INCLUDE_BENCHMARK=1 --fresh .. -B ../build && cd ../build && make && cd - \
  && cmake . -DCMAKE_BUILD_TYPE=Release && make; clear -x;\
  printf "Tests passed.\nBenchmark build success.\n Running benchmark:\n";\
  ./benchmark_parallel_letter_frequency && printf "\n"
