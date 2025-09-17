FLAGS = -std=c++23 -I./ -I./src -g -O0

OUTPUT = libAlcubierre.a

BASE_DIR = src/core
MODULE_DIR = src/module

BASE_SRCS = $(shell find $(BASE_DIR) -name source.cpp)
MODULE_SRCS = $(shell find $(MODULE_DIR) -name source.cpp)

BASE_OBJS = $(BASE_SRCS:.cpp=.o)
MODULE_OBJS = $(MODULE_SRCS:.cpp=.o)
ALL_OBJS = $(BASE_OBJS) $(MODULE_OBJS)

default: $(OUTPUT)

full: clean $(OUTPUT)

$(OUTPUT): $(ALL_OBJS)
	ar rcs build/$@ $^
	@cp -u $(BASE_DIR)/PublicInterface/public.h build/AlcubierreEngine.h

%.o: %.cpp
	g++ $(FLAGS) -o $@ -c $< > makelog 2>&1

%: $(BASE_DIR)/%/source.cpp
	rm $(BASE_DIR)/$@/source.o
	g++ $(FLAGS) -o $(BASE_DIR)/$@/source.o -c $< > makelog 2>&1

%: $(MODULE_DIR)/%/source.cpp
	rm $(MODULE_DIR)/$@/source.o
	g++ $(FLAGS) -o $(MODULE_DIR)/$@/source.o -c $< > makelog 2>&1

clean:
	rm $(shell find src -name '*.o') build/$(OUTPUT) build/AlcubierreEngine.h