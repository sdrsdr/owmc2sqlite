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
#ifdef FEATURE_DEDUP
	static inline int64_t latlon2id(int lat, int lon) {
		//result has  digital structure SNNNNNNTTTTT
		//where S is lat,lon signs encoded, NNNNNN is lon digits TTTTT is lat digits 
		//S is one of 1,2,3,4  
		//       100000 lon shifter so lat can fit in lower part
		//max lat=90000 (5dig)
		//max lon=180000 (6dig)
		//       100000000000 sign shhifter so lat/lon can fit below
		//max id=418000090000
		//log2(418000090000)=38.605 id easily fits in 64bit
		if (lat>0) {
			if (lon>0) {
				return 100000000000+((int64_t)lon*100000)+lat;
			} else {
				//lon=-lon;
				return 200000000000+((int64_t)lon*-100000)+lat;
			}
		} else {
			//lat=-lat;
			if (lon>0) {
				return 300000000000+((int64_t)lon*100000)+(-lat);
			} else {
				//lon=-lon;
				return 400000000000+((int64_t)lon*-100000)+(-lat);
			}
		}

	}
#endif
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
			"id NUMBER "
#ifdef FEATURE_DEDUP
				" PRIMARY KEY"
#endif			
			", name TEXT,"

#ifdef FEATURE_DEDUP
			"altname TEXT DEFAULT '',"
#endif			
			"state TEXT, "
			"country TEXT, "
			"lat INTEGER, "
			"lon INTEGER "
		") "
#ifdef FEATURE_DEDUP
		" WITHOUT ROWID"
#endif		
		,
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
		"VALUES             (?1, ?2 ,  ?3 ,  ?4   , ?5, ?6 ) "
#ifdef FEATURE_DEDUP
		"ON CONFLICT (id) DO UPDATE SET altname=CASE "
		"  WHEN cities.name=?2 THEN cities.altname "
		"  WHEN cities.altname='' THEN ?2 "
		"  ELSE ?2 || ';' || cities.altname "
		"END "
#endif
		,
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
#ifdef FEATURE_DEDUP
	id=latlon2id(lat,lon);
#endif	
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