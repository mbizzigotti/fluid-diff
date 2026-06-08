
FLAGS  := -Wall -Wextra -Wpedantic # -Werror
RAYLIB := $(shell pkg-config --cflags --libs raylib)
ENZYME := -fplugin=/opt/homebrew/Cellar/enzyme/0.0.263/lib/ClangEnzyme-22.dylib -I/opt/homebrew/Cellar/enzyme/0.0.263/include/

smokey: main.c
	clang -O3 ${FLAGS} ${RAYLIB} ${ENZYME} $< -o $@

examples: example_image

# clang -O2 ${ENZYME} $< -o $@
%: %.cpp
	clang++ -O3 ${FLAGS} ${RAYLIB} ${ENZYME} $< -o $@

%: %.c
	clang -O3 ${FLAGS} ${RAYLIB} ${ENZYME} $< -o $@

# DEBUGGING:

%.ll: %.c
	clang ${FLAGS} ${RAYLIB} $< -S -emit-llvm -o $@ -O2 -fno-vectorize -fno-slp-vectorize -fno-unroll-loops

%.diff.ll: %.ll
	opt $< --load-pass-plugin=/opt/homebrew/Cellar/enzyme/0.0.263/lib/LLVMEnzyme-22.dylib -passes=enzyme -o $@ -S

%.test: %.diff.ll
	clang $< -O3 -o $@