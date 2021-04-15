#*********
# Copyright (c) 2021 Stoian Ivanov 
#  This file is under a MIT License
#*********

deftarget:build/owmc2sqlite


ifndef SQLITE3_LIBS
ifeq (,$(wildcard ./sqlite3.make))
$(info Generating sqlite3.make via pkgconf...)
$(file >./sqlite3.make,SQLITE3_LIBS:=$(shell pkgconf sqlite3 --libs))
$(file >>./sqlite3.make,SQLITE3_CFLAGS:=$(shell pkgconf sqlite3 --cflags))
endif
include ./sqlite3.make
endif

ifndef SQLITE3_LIBS
$(error SQLITE3_LIBS not defined and ./sqlite3.make is not generated properly. Do you have libsqlite3 dev package installed?)
endif


ifndef RAPIDJSON_CFLAGS
ifeq (,$(wildcard ./rapidjson.make))
$(info Generating rapidjson.make via pkgconf...)
$(file >>./rapidjson.make,RAPIDJSON_CFLAGS:=$(shell pkgconf RapidJSON  --cflags))
endif
include ./rapidjson.make
endif



ifeq ($(wildcard build/.),)
ifeq ($(XDG_RUNTIME_DIR),)
$(info creating simple build dir)
$(shell mkdir build)
else 
$(info creating tmpfs build dir)
$(shell mkdir $(XDG_RUNTIME_DIR)/owmc2sqlite_build)
$(shell ln -s $(XDG_RUNTIME_DIR)/owmc2sqlite_build build)

endif
endif

$(info SQLITE3_LIBS=${SQLITE3_LIBS} SQLITE3_CFLAGS=${SQLITE3_CFLAGS} RAPIDJSON_CFLAGS=${RAPIDJSON_CFLAGS} )


#CFLAGS=-Wall -g -Og ${SQLITE3_CFLAGS} ${RAPIDJSON_CFLAGS}
CFLAGS=-Wall -O3 ${SQLITE3_CFLAGS} ${RAPIDJSON_CFLAGS}
CXXFLAGS=-std=c++17

.ONESHELL:
.PHONY: clean distclean 


build/%.o: %.cpp
	@echo [CPPC $<]
	g++ -c -o $@ $(realpath $<) ${CFLAGS} ${CXXFLAGS} -MMD -MP -MF ${@:.o=.d}

LINK=g++ -o $@ $^ ${SQLITE3_LIBS}

build/owmc2sqlite: build/main.o build/json_sax.o build/sqlite_be.o
	@echo [LN $@]
	${LINK}

clean:
	rm -f build/* 

distclean: clean
	rm ./rapidjson.make ./sqlite3.make


ifneq ($(MAKECMDGOALS),clean)
-include build/*.d
endif  