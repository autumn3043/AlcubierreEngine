COMPILEFLAGS = -std=c++23 -O2 -I./ -I./src -shared -fPIC
LINKFLAGS = -lglfw -lvulkan -ldl -lpthread -lwayland-client -lwayland-cursor -lwayland-egl

OUTPUT = libAlcubierre.so
CONFIGPREFIX = ./src/core/DataManager/default
TARGET = $(shell find src -name '*.cpp')

.PHONY: test build dataformat clean

dataformat:
	@xxd -i -n formattedjson_enginedata $(CONFIGPREFIX)/enginedata.json > $(CONFIGPREFIX)/enginedata_json.h

build:
	$(MAKE) dataformat
	g++ $(COMPILEFLAGS) -o $(OUTPUT) $(TARGET) $(LINKFLAGS) > makelog 2>&1

clean:
	rm $(OUTPUT)