COMPILEFLAGS = -std=c++23 -O2 -I./ -I./src -shared -fPIC
LINKFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

OUTPUT = libAlcubierre.so
CONFIGPREFIX = ./src/module/DataManager/default
TARGET = $(shell find src -name '*.cpp')

.PHONY: test build dataformat clean

dataformat:
	@xxd -i -n formattedjson_userdata $(CONFIGPREFIX)/data/userdata.json > $(CONFIGPREFIX)/format/userdata_json.h
	@xxd -i -n formattedjson_appdata $(CONFIGPREFIX)/data/appdata.json > $(CONFIGPREFIX)/format/appdata_json.h
	@xxd -i -n formattedjson_enginedata $(CONFIGPREFIX)/data/enginedata.json > $(CONFIGPREFIX)/format/enginedata_json.h

build:
	$(MAKE) dataformat 
	g++ $(COMPILEFLAGS) -o $(OUTPUT) $(TARGET) $(LINKFLAGS) > makelog 2>&1

clean:
	rm $(OUTPUT)