/*********
* Copyright (c) 2021 Stoian Ivanov 
*  This file is under a MIT License
**********/

#include <cstddef>
#include <iostream>
#include "sqlite_be.hpp"

#include <sqlite3.h>
using namespace std;

namespace SQLITE_BE {
	sqlite3 *db;
	sqlite3_stmt *insst;
	int insert_errs;
}

using namespace SQLITE_BE;

/*
[
    {
        "id": 833,
        "name": ".....",
        "state": "....",
        "country": "...",
        "coord": {
            "lon": 47.159401,
            "lat": 34.330502
        }
    }
	....
]
*/


//dbname is guaranteed to be non existant
//return 0 to indicate no error
int sqlite_be_init(const char *dbname) { 
	db=NULL; insst=NULL; insert_errs=0;
	if (sqlite3_open_v2(dbname,&db,SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,NULL)!=SQLITE_OK) {
		cout<<"Failed to create database "<<dbname<<" err:"<<(db?sqlite3_errmsg(db):"db is NULL")<<endl;
		if (db) sqlite3_close(db);
		db=NULL;
		return 1;
	}
	if (db==NULL) {
		cout<<"Failed to create database "<<dbname<<" or result but db is NULL"<<endl;
		return 1;
	}
	sqlite3_stmt *stmt=NULL;
	if (sqlite3_prepare_v2(
		db,
		"CREATE TABLE cities ( "
			"id NUMBER, "
			"name TEXT,"
			"state TEXT, "
			"country TEXT, "
			"lat INTEGER, "
			"lon INTEGER "
		")",
		-1,
		&stmt,
		NULL
	)!=SQLITE_OK){
		cout<<"Failed to create statement @1 for table cities in "<<dbname<<" err:"<<sqlite3_errmsg(db)<<endl;
		if (stmt) sqlite3_finalize(stmt);
		stmt=NULL;
		if (db) sqlite3_close(db);
		db=NULL;
		return 2;
	};
	if (!stmt) {
		cout<<"Failed to create statement @2 for table creation in "<<dbname<<" err:"<<sqlite3_errmsg(db)<<endl;
		if (db) sqlite3_close(db);
		db=NULL;
		return 3;
	}
	if (sqlite3_step(stmt)!=SQLITE_DONE) {
		cout<<"Failed to create table in "<<dbname<<" err:"<<sqlite3_errmsg(db)<<endl;
		if (stmt) sqlite3_finalize(stmt);
		stmt=NULL;
		if (db) sqlite3_close(db);
		db=NULL;
		return 3;

	}
	if (stmt) sqlite3_finalize(stmt);
	stmt=NULL;

	cout<<"Table cities in "<<dbname<<" created! Let's hope it is in tmpfs or this might take SOME time..."<<endl;
	if (sqlite3_prepare_v2(
		db, //                1  2     3     4      5   6
		"INSERT INTO cities (id,name,state,country,lat,lon) "
		"VALUES             ( ?,  ? ,  ?  ,   ?   , ? , ? )",
		-1,
		&insst,
		NULL
	)!=SQLITE_OK){
		cout<<"Failed to create insert statement for table cities in "<<dbname<<" err:"<<sqlite3_errmsg(db)<<endl;
		if (insst) sqlite3_finalize(insst);
		insst=NULL;
		if (db) sqlite3_close(db);
		db=NULL;
		return 4;
	};
	return 0;
}

int sqlite_be_fin(void) {
	if (insert_errs>0) {
		cout<<"sqlite backend got "<<insert_errs<<" while inserting data!"<<endl;
	}
	if (insst) sqlite3_finalize(insst);
	insst=NULL;


	sqlite3_stmt *stmt=NULL;
	if (sqlite3_prepare_v2(
		db,
		"CREATE INDEX lat_lon ON cities ( lat,lon )",
		-1,
		&stmt,
		NULL
	)!=SQLITE_OK){
		cout<<"Failed to create statement @1 for index creation err:"<<sqlite3_errmsg(db)<<endl;
		if (stmt) sqlite3_finalize(stmt);
		stmt=NULL;
		if (db) sqlite3_close(db);
		db=NULL;
		return 2;
	};
	if (!stmt) {
		cout<<"Failed to create statement @2 for index creation err:"<<sqlite3_errmsg(db)<<endl;
		if (db) sqlite3_close(db);
		db=NULL;
		return 3;
	}
	cout<<"index creation starts"<<endl;
	if (sqlite3_step(stmt)!=SQLITE_DONE) {
		cout<<"Failed to create index err:"<<sqlite3_errmsg(db)<<endl;
		if (stmt) sqlite3_finalize(stmt);
		stmt=NULL;
		if (db) sqlite3_close(db);
		db=NULL;
		return 3;

	}
	cout<<"index creation ends"<<endl;
	if (stmt) sqlite3_finalize(stmt);
	stmt=NULL;


	if (db) sqlite3_close(db);
	db=NULL;
	return insert_errs;
}

void sqlite_be_store(int64_t id, const char *city, const char* state, const char* country, int lat, int lon) {
	sqlite3_bind_int64(insst,1,id);
	sqlite3_bind_text(insst,2,city,-1, SQLITE_STATIC);
	sqlite3_bind_text(insst,3,state,-1, SQLITE_STATIC);
	sqlite3_bind_text(insst,4,country,-1, SQLITE_STATIC);
	sqlite3_bind_int(insst,5,lat);
	sqlite3_bind_int(insst,6,lon);
	if (sqlite3_step(insst)!=SQLITE_DONE) {
		cout<<"sqlite_be_store got err:"<<sqlite3_errmsg(db)<<endl;
		insert_errs++;
	}
	sqlite3_reset(insst);
}