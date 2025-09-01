BUILD_DIR = ../libAlcubierre/build

COMPILEFLAGS = -std=c++23 -g -O0 -I$(BUILD_DIR)
LINKFLAGS = -lglfw -lvulkan -ldl -lpthread -lwayland-client -lwayland-cursor -lwayland-egl 

OUTPUT = Sandbox
TARGET = ./main.cpp

test: 

build:
	g++ $(COMPILEFLAGS) -o $(OUTPUT) $(TARGET) -Wl,--whole-archive $(BUILD_DIR)/libAlcubierre.a -Wl,--no-whole-archive $(LINKFLAGS) > makelog 2>&1

clean:
	rm $(OUTPUT)