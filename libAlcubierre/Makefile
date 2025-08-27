FLAGS = -std=c++23 -O2 -I./ -I./src -g -O0

OUTPUT = libAlcubierre.a

BASE_DIR = src/core
MODULE_DIR = src/module

LINKED_LIBS = -lglfw -lvulkan -ldl -lpthread -lwayland-client -lwayland-cursor -lwayland-egl

BASE_SRCS = $(shell find $(BASE_DIR) -name source.cpp)
BASE_OBJS = $(patsubst $(BASE_DIR)/%/source.cpp,%.o,$(BASE_SRCS))
MODULE_SRCS = $(shell find $(MODULE_DIR) -name source.cpp)
MODULE_OBJS = $(patsubst $(MODULE_DIR)/%/source.cpp,%.o,$(MODULE_SRCS))
ALL_OBJS = $(BASE_OBJS) ConfigManager.o

engine: $(ALL_OBJS)
	ar rcs build/$(OUTPUT) $(shell find src -name '*.o')
	cp $(BASE_DIR)/PublicInterface/public.h build/AlcubierreEngine.h

%.o: $(BASE_DIR)/%/source.cpp
	g++ $(FLAGS) -o $(<D)/$@ -c $< > makelog 2>&1

%.o: $(MODULE_DIR)/%/source.cpp
	g++ $(FLAGS) -o $(<D)/$@ -c $< > makelog 2>&1

clean:
	rm $(shell find src -name '*.o')