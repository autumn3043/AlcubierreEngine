FLAGS = -std=c++23 -I./src -g -O0 -MMD -MF $*.d

OUTPUT = libAlcubierre.a

BASE_DIR = src/core
MODULE_DIR = src/module
EXTERNAL_DIR = src/extern

BASE_SRCS = $(shell find $(BASE_DIR) -name source.cpp)
MODULE_SRCS = $(shell find $(MODULE_DIR) -name source.cpp)
EXTERNAL_SRCS = $(shell find $(EXTERNAL_DIR) -name *.cpp)

BASE_OBJS = $(BASE_SRCS:.cpp=.o)
MODULE_OBJS = $(MODULE_SRCS:.cpp=.o)
EXTERNAL_OBJS = $(EXTERNAL_SRCS:.cpp=.o)
ALL_OBJS = $(BASE_OBJS) $(MODULE_OBJS) $(EXTERNAL_OBJS)

default: $(OUTPUT)

full: clean $(OUTPUT)

$(OUTPUT): $(ALL_OBJS)
	@cp -u $(BASE_DIR)/PublicInterface/public.h build/AlcubierreEngine.h
	@{ for f in $(EXTERNAL_DIR)/*; do if [ -f "$$f/LICENSE" ]; then name=$${f##*/}; printf '%s\n\n' "<----- $$name ----->"; cat "$$f/LICENSE"; printf '\n\n'; fi; done; } > build/THIRD_PARTY_LICENSES.txt
	ar rcs build/$@ $^

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

-include $(BASE_OBJS:.o=.d)
-include $(MODULE_OBJS:.o=.d)