cmake_minimum_required(VERSION 3.13)
project(Fluid LANGUAGES C CXX)

# Using Compiler-plugin approach (ClangEnzyme)
# -> differentiates per compilation unit

find_package(Enzyme REQUIRED CONFIG)
find_package(raylib REQUIRED)
find_package(NLopt REQUIRED)
find_library(COCOA_LIBRARY Cocoa) # MacOS only
find_package(Vulkan REQUIRED)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(Slang)

# MAIN Smoke Simulation

add_executable(smokey src/smokey.c src/3rdparty/lbfgs.c)

target_compile_options(smokey PRIVATE -march=native)
target_compile_definitions(smokey PRIVATE "LBFGS_FLOAT=32")
target_link_libraries(smokey PRIVATE raylib ClangEnzymeFlags NLopt::nlopt)

# GPU Smoke Simulation

add_executable(smokey-gpu src/smokey-gpu.c)
if(COCOA_LIBRARY)
    target_link_libraries(smokey-gpu PRIVATE ${COCOA_LIBRARY})
endif()
target_link_libraries(smokey-gpu PRIVATE Vulkan::Vulkan)

add_slang_shaders(smokey-gpu SHADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/Render.slang
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/ApplyControl.slang
)

# TOOLS

add_executable(controls src/controls.c)
add_executable(keyframe src/keyframe.c)
add_executable(config src/config.c)

target_link_libraries(keyframe PRIVATE raylib)
target_link_libraries(config PRIVATE raylib)

# TESTS

add_executable(gaussian tests/gaussian.c)
add_executable(test_nlopt tests/nlopt.c)
add_executable(text_with_dots tests/text_with_dots.c)

target_link_libraries(gaussian PRIVATE raylib ClangEnzymeFlags)
target_link_libraries(test_nlopt PRIVATE NLopt::nlopt m)
target_link_libraries(text_with_dots PRIVATE raylib)
