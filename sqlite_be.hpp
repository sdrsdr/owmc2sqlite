/*********
* Copyright (c) 2021 Stoian Ivanov 
*  This file is under a MIT License
**********/

#pragma once
#include <stdint.h>

//dbname is guaranteed to be non existant
//return 0 to indicate no error
int sqlite_be_init(const char *dbname);

int sqlite_be_fin(void);
void sqlite_be_store(int64_t id, const char *city, const char* state, const char* country, int lat, int lon);