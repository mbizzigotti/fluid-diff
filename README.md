# fluid-diff
Differentiable fluid simulation

# Build Instructions

## Prerequisite Libraries
- [Enzyme](https://enzyme.mit.edu/)
- [NLopt](https://nlopt.readthedocs.io/)
- [raylib](https://raylib.com/)

## Setup
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
```
NOTE: You must specify the compiler because EnzymeAD only works with LLVM.

## Build
```bash
cmake --build build
```
