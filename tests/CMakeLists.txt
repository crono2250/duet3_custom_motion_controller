cmake_minimum_required(VERSION 3.24)

project(openpnp_motion_host_tests LANGUAGES C)

include(CTest)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

function(add_motion_test target)
  add_executable(${target} ${ARGN})
  target_include_directories(${target} PRIVATE ../firmware/include)
  target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  add_test(NAME ${target} COMMAND ${target})
endfunction()

add_motion_test(clock_sync_test
  clock_sync_test.c
  ../firmware/src/clock_sync.c
)

add_motion_test(step_engine_test
  step_engine_test.c
  ../firmware/src/step_engine.c
)

add_motion_test(motion_gateway_test
  motion_gateway_test.c
  ../firmware/src/clock_sync.c
  ../firmware/src/motion_gateway.c
  ../firmware/src/step_engine.c
)
