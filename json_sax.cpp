/*********
* Copyright (c) 2021 Stoian Ivanov 
*  This file is under a MIT License
**********/

#include <iostream>
#include <iomanip>
#include <cstring>

#include <rapidjson/rapidjson.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/reader.h>

#include "json_sax.hpp"
#include "sqlite_be.hpp"

using namespace std;
using rapidjson::SizeType;

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


namespace JSON_SAX { 


//namsepaced "globals" for fastes access 
int l0_arr_items=-1;
int cities=0;
int disabled_cities=0;
int lvl=0;
bool in_l1_arr=false;
bool in_l2_obj=false;
bool got_l2_coord_key=false;
bool in_l3_coord_obj=false;
enum Expectations_t {
	id=0,
	name,
	state,
	country,
	lon,
	lat,
	ignore,
};

unsigned data_found=0;
const unsigned complete_data_mask=(1<<id)|(1<<name)|(1<<state)|(1<<country)|(1<<lon)|(1<<lat);
const unsigned str_buf_sz=1023;
Expectations_t exp=ignore;

int64_t city_id;
double city_lon;
double city_lat;
char city_name[str_buf_sz+1]; 
char city_state[str_buf_sz+1];
char city_country[str_buf_sz+1];

void reset_city_data(){
	city_id=0;
	city_lat=0;
	city_lon=0;
	city_name[0]=0;
	city_state[0]=0;
	city_country[0]=0;
}

#ifdef FEATURE_HIST
unsigned lon_hist[36];
unsigned lat_hist[18];
#endif

void flush_owm_city(){
	if ((data_found&complete_data_mask)!=complete_data_mask) {
		cout<<"...flush_owm_city but data is incomplete ("<<hex<<data_found<<") stored name:"<<city_name<<" stored id:"<<city_id<<endl;
		reset_city_data();
		return;
	}
	cities++;
	if (city_id==INT64_MIN) {
		disabled_cities++;
		reset_city_data();
		return;
	}

	if (city_lat<-90 || city_lat>90) {
		cout<<"...flush_owm_city but data seems invalid ("<<hex<<data_found<<") name:"<<city_name<<" id:"<<city_id<<" out of range lat:"<<city_lat<<endl;
		reset_city_data();
		return;
	}
	if (city_lon<-180 || city_lon>180) {
		cout<<"...flush_owm_city but data seems invalid ("<<hex<<data_found<<") name:"<<city_name<<" id:"<<city_id<<" out of range lat:"<<city_lat<<endl;
		reset_city_data();
		return;
	}

#ifdef FEATURE_HIST
	unsigned lon_hist_i=((unsigned)(city_lon+180))/10;
	if (lon_hist_i<0 || lon_hist_i>35) {
		cout<<"lon_hist_i fails at "<<lon_hist_i<<" with lon:"<<city_lon<<" for id:"<<city_id<<endl;
	} else {
		lon_hist[lon_hist_i]++;
	}

	unsigned lat_hist_i=((unsigned)(city_lat+90))/10;
	if (lat_hist_i<0 || lat_hist_i>18) {
		cout<<"lat_hist_i fails at "<<lat_hist_i<<" with lon:"<<city_lat<<" for id:"<<city_id<<endl;
	} else {
		lat_hist[lat_hist_i]++;
	}
#endif

	sqlite_be_store(city_id,city_name,city_state, city_country, city_lat*1000,city_lon*1000);

/* TODO:
test.m 
----------------
R = 6371e3/1000; 
lon_km=100;
tbl=[];
for i=0:8
 deg4lon_km=rad2deg(lon_km/(R*cos(deg2rad(i*10))));
 tbl=[tbl; i*10,deg4lon_km];
endfor 
tbl
---------------

results in
---------------
tbl =

    0.00000    0.89932
   10.00000    0.91320
   20.00000    0.95704
   30.00000    1.03845
   40.00000    1.17398
   50.00000    1.39910
   60.00000    1.79864
   70.00000    2.62944
   80.00000    5.17899
----------------

use tbl and duplicate evert city near 180 lon bondery for given lat so we don't have 
to wory about wrapping intervals in sql selects latter 

tbl column 1 is lat boundery, column 2 is lon degrees needed for 100KM distance 
over the given parallel line assuming spherical earth 

*/


	//cout<<"owm city:"<<city_name<<" id:"<<city_id<<" country:"<<city_country<<" state:"<<city_state<<" lon:"<<city_lon<<" lat:"<<city_lat<<endl;
	reset_city_data();
}

struct Handler;
//singleton instance
Handler* handler_singleton=NULL;

//singleton class to handle SAX events
struct Handler {

	//sigleton "constructor"
	static Handler& GetHandler() { 
		if (handler_singleton==NULL) handler_singleton=new Handler();
		reset();
		return *handler_singleton;
	}

	//reset sigleton so new parse can start just for completenes as is never used here
	static void reset() {
		lvl=0;
		in_l1_arr=false;
		in_l2_obj=false;
		got_l2_coord_key=false;
		in_l3_coord_obj=false;
		data_found=0;
		exp=ignore;
		cities=0;
		l0_arr_items=-1;
		disabled_cities=0;
#ifdef FEATURE_HIST		
		memset(lon_hist,0,sizeof(lon_hist));
		memset(lat_hist,0,sizeof(lat_hist));
#endif
	}

	//we simply ignore NULL,bool,RawNumber types
	bool Null() { exp=ignore; return true; }
	bool Bool(bool b) { exp=ignore; return true; }
	bool RawNumber(const char* str, SizeType length, bool) { exp=ignore; return true; }

	//asuming city id is int64 this uint64 might be a bit too wide so we also ignore this
	bool Uint64(uint64_t u) {exp=ignore; return true; } 


	//safely converge int, unsigned to int64
	bool Int(int i) { if (exp==ignore) return true; return  Int64(i); } 
	bool Uint(unsigned u) {if (exp==ignore) return true; return Int64(u); }

	bool Int64(int64_t i) { 
		if (exp==id) {
			city_id=i;
			data_found|=(1<<id);
		}
		exp=ignore;
		return true;
	}

	bool Double(double d) {
		if (exp==lat) {
			city_lat=d;
			data_found|=(1<<lat);
		} else if (exp==lon) {
			city_lon=d;
			data_found|=(1<<lon);
		} else if (exp==id) {
			data_found|=(1<<id);
			city_id=INT64_MIN;
		}

		exp=ignore;
		return true;
	}
	
	bool String(const char* str, SizeType length, bool) { 
		if (exp==ignore) return true;
		if (length>str_buf_sz) {
			exp=ignore;
			return true;
		}

		if (exp==name) {
			memcpy(city_name,str,length);
			city_name[length]=0;
			data_found|=(1<<name);
			exp=ignore;
			return true;
		}
		if (exp==country) {
			if (length>0) memcpy(city_country,str,length);
			city_country[length]=0;
			data_found|=(1<<country);
			exp=ignore;
			return true;
		}
		if (exp==state) {
			if (length>0) memcpy(city_state,str,length);
			city_state[length]=0;
			data_found|=(1<<state);
			exp=ignore;
			return true;
		}
		exp=ignore;
		return true;
	}

	bool Key(const char* str, SizeType len, bool) {
		got_l2_coord_key=false;
		if (in_l2_obj) {
			if (len==2 && memcmp(str,"id",2)==0) {
				exp=id;
				return true;
			}
			if (len==4 && memcmp(str,"name",4)==0) {
				exp=name;
				return true;
			}
			if (len==5) {
				//              12345
				if (memcmp(str,"state",5)==0) {
					exp=state;
					return true;
				}
				//              12345
				if (memcmp(str,"coord",5)==0) {
					got_l2_coord_key=true;
					return true;
				}
			}
			//                        1234567
			if (len==7 && memcmp(str,"country",7)==0) {
				exp=country;
				return true;
			}
		}
		if (in_l3_coord_obj && len==3) {
			if (memcmp(str,"lon",3)==0) {
				exp=lon;
				return true;
			}
			if (memcmp(str,"lat",3)==0) {
				exp=lat;
				return true;
			}
		}

		exp=ignore;
		return true;
	}

	bool StartObject() {
		lvl++; 
		if (in_l1_arr){
			if(lvl==2) {
				data_found=0;
				in_l2_obj=true;
			} else if (lvl==3 && got_l2_coord_key) {
				got_l2_coord_key=false;
				in_l3_coord_obj=true;
			}
		}
		return true; 
	}

	bool EndObject(SizeType memberCount) {
		if (in_l1_arr && lvl==2) {
			in_l2_obj=false;
			flush_owm_city();
		} else if (in_l3_coord_obj && lvl==3) {
			in_l3_coord_obj=false;
		}
		lvl--;
		return true;
	}

	bool StartArray() {
		lvl++; 
		if (lvl==1) in_l1_arr=true;
		return true; 
	}
	bool EndArray(SizeType elementCount) {
		if (lvl==1) {
			l0_arr_items=elementCount;
			in_l1_arr=false;
		}
		lvl--; 
		return true; 
	}

private:
	//force singleton by hiding the constructors
	Handler(const Handler& noCopyConstruction);
	Handler& operator=(const Handler& noAssignment);
	Handler() {}
};



} //namespace JSON_SAX 

using namespace JSON_SAX;

void json_sax_parse(FILE *fp) {
	char fstrm_buffer[1024*40];
	rapidjson::FileReadStream fstrm (fp,fstrm_buffer,sizeof(fstrm_buffer));
	rapidjson::Reader reaedr;
	reaedr.Parse(fstrm, JSON_SAX::Handler::GetHandler());
}

int json_sax_fin(void) {
	cout<<"Parsing done! Array items:"<<l0_arr_items<<" cities:"<<cities<<" ("<<disabled_cities<<" disabled) "<<(l0_arr_items==cities?" ALLOK!":" SOMETHING WENT WRONG!")<<endl;
#ifdef FEATURE_HIST
	cout<<"Lon hystogram:"<<endl;
	for(int i=0; i<36; i++) {
		cout<<setw(2)<<i<<":"<<setw(6)<<JSON_SAX::lon_hist[i]<<endl;
	}
	cout<<"Lat hystogram:"<<endl;
	for(int i=0; i<18; i++) {
		cout<<setw(2)<<i<<":"<<setw(6)<<JSON_SAX::lat_hist[i]<<endl;
	}
#endif
	if (JSON_SAX::l0_arr_items!=JSON_SAX::cities) return 3;
	return 0;
}

