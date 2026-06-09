cmake_minimum_required(VERSION 3.13)
project(Fluid LANGUAGES C CXX)

# Using Compiler-plugin approach (ClangEnzyme)
# -> differentiates per compilation unit

find_package(Enzyme REQUIRED CONFIG)
find_package(raylib REQUIRED)
find_package(NLopt REQUIRED)

# MAIN Smoke Simulation

add_executable(smokey src/smokey.c)

target_link_libraries(smokey PRIVATE raylib ClangEnzymeFlags NLopt::nlopt)

# TOOLS

add_executable(controls src/controls.c)
add_executable(keyframe src/keyframe.c)
add_executable(config src/config.c)

target_link_libraries(keyframe PRIVATE raylib)

# TESTS

add_executable(gaussian tests/gaussian.c)
add_executable(test_nlopt tests/nlopt.c)

target_link_libraries(gaussian PRIVATE raylib ClangEnzymeFlags)
target_link_libraries(test_nlopt PRIVATE NLopt::nlopt)
