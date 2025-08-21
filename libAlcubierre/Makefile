COMPILEFLAGS = -std=c++23 -O2 -I./ -I./src -shared -fPIC -g -O0
LINKFLAGS = -lglfw -lvulkan -ldl -lpthread -lwayland-client -lwayland-cursor -lwayland-egl

OUTPUT = libAlcubierre.so
CONFIGPREFIX = ./src/module/DataManager/default
TARGET = $(shell find src -name '*.cpp')
TEMPTARGET = src/core/DebugManager/source.cpp src/core/PublicInterface/source.cpp src/core/Registry/source.cpp src/module/ConfigManager/source.cpp AlcubierreEngine.h

.PHONY: fullbuild build dataformat clean

fullbuild:
	$(MAKE) dataformat
	$(MAKE) build

dataformat:
	@xxd -i -n formattedjson_enginedata $(CONFIGPREFIX)/enginedata.json > $(CONFIGPREFIX)/enginedata_json.h

build:
	g++ $(COMPILEFLAGS) -o $(OUTPUT) $(TEMPTARGET) $(LINKFLAGS) > makelog 2>&1

clean:
	rm $(OUTPUT)