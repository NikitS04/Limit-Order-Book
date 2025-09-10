cmake_minimum_required(VERSION 3.16)
project(lob_engine_zeroalloc LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)

  add_executable(fifo_priority_tests tests/fifo_priority_tests.cpp)
  target_link_libraries(fifo_priority_tests PRIVATE lob)
  add_test(NAME fifo_priority_tests COMMAND fifo_priority_tests)

  add_executable(crossing_multi_level_tests tests/crossing_multi_level_tests.cpp)
  target_link_libraries(crossing_multi_level_tests PRIVATE lob)
  add_test(NAME crossing_multi_level_tests COMMAND crossing_multi_level_tests)

  add_executable(partial_fill_persistence_tests tests/partial_fill_persistence_tests.cpp)
  target_link_libraries(partial_fill_persistence_tests PRIVATE lob)
  add_test(NAME partial_fill_persistence_tests COMMAND partial_fill_persistence_tests)

  add_executable(cancel_head_middle_tail_tests tests/cancel_head_middle_tail_tests.cpp)
  target_link_libraries(cancel_head_middle_tail_tests PRIVATE lob)
  add_test(NAME cancel_head_middle_tail_tests COMMAND cancel_head_middle_tail_tests)

  add_executable(modify_reprice_tests tests/modify_reprice_tests.cpp)
  target_link_libraries(modify_reprice_tests PRIVATE lob)
  add_test(NAME modify_reprice_tests COMMAND modify_reprice_tests)

  add_executable(market_no_liquidity_tests tests/market_no_liquidity_tests.cpp)
  target_link_libraries(market_no_liquidity_tests PRIVATE lob)
  add_test(NAME market_no_liquidity_tests COMMAND market_no_liquidity_tests)

  add_executable(index_consistency_tests tests/index_consistency_tests.cpp)
  target_link_libraries(index_consistency_tests PRIVATE lob)
  add_test(NAME index_consistency_tests COMMAND index_consistency_tests)

  add_executable(snapshot_every_event_tests tests/snapshot_every_event_tests.cpp)
  target_link_libraries(snapshot_every_event_tests PRIVATE lob)
  add_test(NAME snapshot_every_event_tests COMMAND snapshot_every_event_tests)

  add_executable(price_limit_respect_tests tests/price_limit_respect_tests.cpp)
  target_link_libraries(price_limit_respect_tests PRIVATE lob)
  add_test(NAME price_limit_respect_tests COMMAND price_limit_respect_tests)

  add_executable(best_px_helpers_tests tests/best_px_helpers_tests.cpp)
  target_link_libraries(best_px_helpers_tests PRIVATE lob)
  add_test(NAME best_px_helpers_tests COMMAND best_px_helpers_tests)

endif()
add_library(lob src/book.cpp src/matcher.cpp src/metrics.cpp)
target_include_directories(lob PUBLIC include)
target_compile_options(lob PRIVATE
  $<$<CONFIG:Release>:-O3 -DNDEBUG>
  $<$<CONFIG:RelWithDebInfo>:-O2 -g>
  $<$<CONFIG:Debug>:-g -fsanitize=address,undefined>
)
target_link_options(lob PRIVATE
  $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)
add_executable(lob_bin src/main.cpp)
target_link_libraries(lob_bin PRIVATE lob)
set_target_properties(lob_bin PROPERTIES OUTPUT_NAME lob)
include(CTest)
if(BUILD_TESTING)
  add_executable(book_tests tests/book_tests.cpp)
  target_link_libraries(book_tests PRIVATE lob)
  add_test(NAME book_tests COMMAND book_tests)
  add_executable(matcher_tests tests/matcher_tests.cpp)
  target_link_libraries(matcher_tests PRIVATE lob)
  add_test(NAME matcher_tests COMMAND matcher_tests)

  add_executable(fifo_priority_tests tests/fifo_priority_tests.cpp)
  target_link_libraries(fifo_priority_tests PRIVATE lob)
  add_test(NAME fifo_priority_tests COMMAND fifo_priority_tests)

  add_executable(crossing_multi_level_tests tests/crossing_multi_level_tests.cpp)
  target_link_libraries(crossing_multi_level_tests PRIVATE lob)
  add_test(NAME crossing_multi_level_tests COMMAND crossing_multi_level_tests)

  add_executable(partial_fill_persistence_tests tests/partial_fill_persistence_tests.cpp)
  target_link_libraries(partial_fill_persistence_tests PRIVATE lob)
  add_test(NAME partial_fill_persistence_tests COMMAND partial_fill_persistence_tests)

  add_executable(cancel_head_middle_tail_tests tests/cancel_head_middle_tail_tests.cpp)
  target_link_libraries(cancel_head_middle_tail_tests PRIVATE lob)
  add_test(NAME cancel_head_middle_tail_tests COMMAND cancel_head_middle_tail_tests)

  add_executable(modify_reprice_tests tests/modify_reprice_tests.cpp)
  target_link_libraries(modify_reprice_tests PRIVATE lob)
  add_test(NAME modify_reprice_tests COMMAND modify_reprice_tests)

  add_executable(market_no_liquidity_tests tests/market_no_liquidity_tests.cpp)
  target_link_libraries(market_no_liquidity_tests PRIVATE lob)
  add_test(NAME market_no_liquidity_tests COMMAND market_no_liquidity_tests)

  add_executable(index_consistency_tests tests/index_consistency_tests.cpp)
  target_link_libraries(index_consistency_tests PRIVATE lob)
  add_test(NAME index_consistency_tests COMMAND index_consistency_tests)

  add_executable(snapshot_every_event_tests tests/snapshot_every_event_tests.cpp)
  target_link_libraries(snapshot_every_event_tests PRIVATE lob)
  add_test(NAME snapshot_every_event_tests COMMAND snapshot_every_event_tests)

  add_executable(price_limit_respect_tests tests/price_limit_respect_tests.cpp)
  target_link_libraries(price_limit_respect_tests PRIVATE lob)
  add_test(NAME price_limit_respect_tests COMMAND price_limit_respect_tests)

  add_executable(best_px_helpers_tests tests/best_px_helpers_tests.cpp)
  target_link_libraries(best_px_helpers_tests PRIVATE lob)
  add_test(NAME best_px_helpers_tests COMMAND best_px_helpers_tests)

endif()
