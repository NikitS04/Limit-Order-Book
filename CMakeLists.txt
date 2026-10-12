cmake_minimum_required(VERSION 3.16)
project(lob_engine LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_library(lob src/book.cpp src/matcher.cpp)
target_include_directories(lob PUBLIC include)
target_compile_options(lob PRIVATE
  $<$<CONFIG:Release>:-O3>
  $<$<CONFIG:RelWithDebInfo>:-O2 -g>
  $<$<CONFIG:Debug>:-g -fsanitize=address,undefined>
)
target_link_options(lob PUBLIC
  $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)

find_package(Threads REQUIRED)
add_executable(lob_bin src/main.cpp)
target_link_libraries(lob_bin PRIVATE lob Threads::Threads)
set_target_properties(lob_bin PROPERTIES OUTPUT_NAME lob)

# Core-only matching benchmark (no threads, no I/O in the timed region)
add_executable(lob_bench bench/core_bench.cpp)
target_link_libraries(lob_bench PRIVATE lob)
target_compile_options(lob_bench PRIVATE $<$<CONFIG:Release>:-O3>)

include(CTest)
if(BUILD_TESTING)
  # Tests use assert(). -UNDEBUG keeps them active in Release builds; without
  # it every assert compiles to nothing and the tests pass trivially.
  function(add_lob_test name)
    add_executable(${name} tests/${name}.cpp)
    target_link_libraries(${name} PRIVATE lob)
    target_compile_options(${name} PRIVATE -UNDEBUG)
    add_test(NAME ${name} COMMAND ${name})
  endfunction()

  foreach(t
      book_tests
      matcher_tests
      fifo_priority_tests
      crossing_multi_level_tests
      partial_fill_persistence_tests
      cancel_head_middle_tail_tests
      modify_reprice_tests
      modify_cross_tests
      modify_priority_tests
      market_no_liquidity_tests
      index_consistency_tests
      snapshot_every_event_tests
      price_limit_respect_tests
      best_px_helpers_tests
      level_total_tests
      pool_exhaustion_tests
      invariants_fuzz_tests)
    add_lob_test(${t})
  endforeach()
endif()
