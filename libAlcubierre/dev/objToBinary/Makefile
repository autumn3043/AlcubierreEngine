COMPILER = g++
COMPILEFLAGS = -std=c++23 -g -O3

DEPENDENCIES = deps/xxhash.c deps/tiny_obj_loader.cc

default:
	$(COMPILER) $(COMPILEFLAGS) ./main.cpp $(DEPENDENCIES) -o ./objToBinary.bin > makelog 2>&1