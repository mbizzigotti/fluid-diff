cmake_minimum_required(VERSION 3.13)
project(Fluid LANGUAGES C CXX)

# Using Compiler-plugin approach (ClangEnzyme)
# -> differentiates per compilation unit

find_package(Enzyme REQUIRED CONFIG)
find_package(Raylib REQUIRED)

add_executable(gaussian src/gaussian.c)
target_link_libraries(gaussian PRIVATE raylib ClangEnzymeFlags)

add_executable(smokey src/smokey.c)
target_link_libraries(smokey PRIVATE raylib ClangEnzymeFlags)

add_executable(controls src/controls.c)

add_executable(keyframe src/keyframe.c)
target_link_libraries(keyframe PRIVATE raylib)
