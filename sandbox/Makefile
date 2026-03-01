# ---> YOU <--- have to configure these:
ENGINE_DIR = ../libAlcubierre/build
BUILD_DIR = build
SUPPRESSIONS_FILE = leaks.supp

PLATFORM ?= dev

#DEV
ifeq ($(PLATFORM), dev)
	COMPILER = g++
	COMPILEFLAGS = -fsanitize=address -std=c++23 -g -O0 -I$(ENGINE_DIR)
	OUTPUT = Sandbox.bin
endif

#RELEASE
ifeq ($(PLATFORM), linux)
	COMPILER = g++
	COMPILEFLAGS = -std=c++23 -g -O0 -I$(ENGINE_DIR)
	OUTPUT = Sandbox.bin
endif

#To build for windows you must have the relevant build libraries to link against. Currently these are not simply provided by the maintainers, you will have to build GLFW and Vulkan for windows yourself and link them here
#Headers can be shared across platforms but the actual loaders are platform-dependent
#Anybody who needs this should just ask me directly for the relevant cmakes. Trust me, its not worth the headache 
ifeq ($(PLATFORM), windows)
	COMPILER = x86_64-w64-mingw32-g++
	COMPILEFLAGS = -std=c++23 -g -O0 -I$(ENGINE_DIR) -I/home/autumn/.lib/windows/include
	LINKFLAGS = -L/home/autumn/.lib/windows/lib
	OUTPUT = Sandbox.exe
endif

TARGET = ./main.cpp

ASSET_SRC_DIR = ./assets
ASSET_OBJS := $(wildcard $(ASSET_SRC_DIR)/*.obj)
ASSET_MTLS := $(wildcard $(ASSET_SRC_DIR)/*.mtl)
ASSET_BLOBS := $(patsubst $(ASSET_SRC_DIR)/%.obj,$(BUILD_DIR)/assets/%.blob,$(ASSET_OBJS))

default: $(ASSET_BLOBS) $(OUTPUT)

$(OUTPUT):
	$(COMPILER) $(COMPILEFLAGS) $(TARGET) -L$(ENGINE_DIR) -lAlcubierre -Wl,-rpath,'$$ORIGIN' $(LINKFLAGS) -o $(BUILD_DIR)/$(OUTPUT) > makelog 2>&1
	cp -u $(ENGINE_DIR)/libAlcubierre.so $(BUILD_DIR)/libAlcubierre.so
	cp -u settings.json $(BUILD_DIR)/settings.json

$(BUILD_DIR)/assets/%.blob: $(ASSET_SRC_DIR)/%.obj
	@\
	OBJFILE=$(ASSET_SRC_DIR)/$*.obj; \
	MTLFILE=$(ASSET_SRC_DIR)/$*.mtl; \
	if [ -f $$MTLFILE ]; then \
		$(ENGINE_DIR)/modelPrecompiler.bin --quiet $$OBJFILE $$MTLFILE $@; \
	else \
		$(ENGINE_DIR)/modelPrecompiler.bin --quiet $$OBJFILE $@; \
	fi

test: $(ASSET_BLOBS) $(OUTPUT)
	cd $(BUILD_DIR) && LSAN_OPTIONS=suppressions=$(SUPPRESSIONS_FILE) ./$(OUTPUT)

clean:
	rm $(BUILD_DIR)/*

.PHONY: assets