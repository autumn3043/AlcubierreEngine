FLAGS = -std=c++23 -I./ -I./src -g -O0

OUTPUT = libAlcubierre.a

BASE_DIR = src/core
MODULE_DIR = src/module

BASE_SRCS = $(shell find $(BASE_DIR) -name source.cpp)
BASE_OBJS = $(patsubst $(BASE_DIR)/%/source.cpp,$(BASE_DIR)/%/build.o,$(BASE_SRCS))
MODULE_SRCS = $(shell find $(MODULE_DIR) -name source.cpp)
MODULE_OBJS = $(patsubst $(MODULE_DIR)/%/source.cpp,$(MODULE_DIR)/%/build.o,$(MODULE_SRCS))
ALL_OBJS = $(BASE_OBJS) $(MODULE_OBJS)

default: clean engine

engine: $(ALL_OBJS)
	rm build/$(OUTPUT)
	ar rcs build/$(OUTPUT) $(shell find src -name '*.o')
	cp $(BASE_DIR)/PublicInterface/public.h build/AlcubierreEngine.h

%/build.o: %/source.cpp
	g++ $(FLAGS) -o $@ -c $< > makelog 2>&1

%: $(BASE_DIR)/%/source.cpp
	rm $(BASE_DIR)/$@/build.o
	g++ $(FLAGS) -o $(BASE_DIR)/$@/build.o -c $< > makelog 2>&1

%: $(MODULE_DIR)/%/source.cpp
	rm $(MODULE_DIR)/$@/build.o
	g++ $(FLAGS) -o $(MODULE_DIR)/$@/build.o -c $< > makelog 2>&1

clean:
	rm $(shell find src -name '*.o') build/$(OUTPUT)