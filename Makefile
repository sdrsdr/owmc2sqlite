#*********
# Copyright (c) 2021 Stoian Ivanov 
#  This file is under a MIT License
#*********

deftarget:build/owmc2sqlite

#COMPILE_FEATURES += -g -Og #emable debug
COMPILE_FEATURES += -O3 #Optimize for speed

#CODE_FEATURES += -DFEATURE_HIST
CODE_FEATURES += -DFEATURE_WRAP

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

$(info CODE_FEATURES=${CODE_FEATURES} COMPILE_FEATURES=${COMPILE_FEATURES} )
$(info SQLITE3_LIBS=${SQLITE3_LIBS} SQLITE3_CFLAGS=${SQLITE3_CFLAGS} RAPIDJSON_CFLAGS=${RAPIDJSON_CFLAGS} )



CFLAGS=${COMPILE_FEATURES} ${CODE_FEATURES} ${SQLITE3_CFLAGS} ${RAPIDJSON_CFLAGS}

.ONESHELL:
.PHONY: clean distclean 


build/%.o: %.cpp
	@echo [CPPC $<]
	g++ -c -o $@ $(realpath $<) ${CFLAGS} -std=c++17 -Wall -MMD -MP -MF ${@:.o=.d}

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