# ---> YOU <--- have to configure these:
ENGINE_DIR = ../libAlcubierre/build
SUPPRESSIONS_FILE = leaks.supp

PLATFORM ?= dev

#DEV
ifeq ($(PLATFORM), dev)
	COMPILER = g++
	COMPILEFLAGS = -fsanitize=address -std=c++23 -g -O3 -I$(ENGINE_DIR)
	LINKFLAGS = -ldl -lpthread -lwayland-client -lwayland-cursor -lwayland-egl
	OUTPUT = Sandbox
endif

#RELEASE
ifeq ($(PLATFORM), linux)
	COMPILER = g++
	COMPILEFLAGS = -std=c++23 -g -O0 -I$(ENGINE_DIR)
	LINKFLAGS = -ldl -lpthread -lwayland-client -lwayland-cursor -lwayland-egl
	OUTPUT = Sandbox
endif

#To build for windows you must have the relevant build libraries to link against. Currently these are not simply provided by the maintainers, you will have to build GLFW and Vulkan for windows yourself and link them here
#Headers can be shared across platforms but the actual loaders are platform-dependent
#Anybody who needs this should just ask me directly for the relevant cmakes. Trust me, its not worth the headache 
#TODO: Windows build of actual engine
ifeq ($(PLATFORM), windows)
	COMPILER = x86_64-w64-mingw32-g++
	COMPILEFLAGS = -std=c++23 -g -O0 -I$(ENGINE_DIR) -I/home/autumn/.lib/windows/include
	LINKFLAGS = -L/home/autumn/.lib/windows/lib -lgdi32 -lstdc++
	OUTPUT = Sandbox.exe
endif

TARGET = ./main.cpp

build:
	$(COMPILER) $(COMPILEFLAGS)  $(TARGET) -L$(ENGINE_DIR) -lAlcubierre -Wl,-rpath,'$(ENGINE_DIR)' $(LINKFLAGS) -o $(OUTPUT)> makelog 2>&1

test: build
	LSAN_OPTIONS=suppressions=$(SUPPRESSIONS_FILE) ./$(OUTPUT)

clean:
	rm $(OUTPUT)