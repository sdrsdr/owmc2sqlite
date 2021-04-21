/*********
* Copyright (c) 2021 Stoian Ivanov 
*  This file is under a MIT License
**********/

#include <sys/stat.h>
#include <sys/unistd.h>

#include <iostream>
#include <cstring>

#include "json_sax.hpp"
#include "sqlite_be.hpp"

using namespace std;


static const size_t fstrm_buffer_sz=1024*40;
int main(int argc, char *argv[]){
#ifdef FEATURE_WRAP
	cout<<"This tool will duplicete some cities near the anti meridian to groom the resulting data set for easy range selections around it"<<endl;
#endif
#ifdef FEATURE_DEDUP
	cout<<"This tool will detect cities with same location and will generate single row for all colliding records; All cities get new id calculated from lat/lon"<<endl;
#endif
	if (argc!=3) {
		cout<<"Params count is "<<argc<<" but we expect it to be 3"<<endl;
		return 1;
	}
	FILE *fp;
	if (strcmp("-",argv[1])==0) {
		fp=stdin;
	} else {
		fp=fopen(argv[1],"r");
		if (!fp) {
			cout<<"Failed to open "<<argv[1]<<" for reading!"<<endl;
			return 2;
		}
	}
	

	struct stat fstats;
	if (stat(argv[2],&fstats)==0) {
		if (unlink(argv[2])!=0) {
			cout<<"Failed to delete "<<argv[2]<<endl;
			return 3;
		};
	}

	if (sqlite_be_init(argv[2])!=0){
		return 4;
	};


	json_sax_parse(fp);

	if (fp!=stdin) fclose(fp);

	int err_be=sqlite_be_fin();
	int err_js=json_sax_fin();

	if (err_be==0 && err_js==0) return 0;
	if (err_be==0 && err_js==1) return 10;
	if (err_be==1 && err_js==0) return 11;
	return 12;

}
