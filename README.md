# owmc2sqlite 

### A tool for importing Open Weather Map  city.list.json to a sqlite3 database

Source JSON files at http://bulk.openweathermap.org/sample/

The code uses sqlite3 libs and https://github.com/Tencent/rapidjson these are available in Ubuntu via `apt install libsqlite3-dev rapidjson-dev`

Currently supported JSON format is 

```
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
```

`lon`, `lat` are multiplied by 1000 and then truncated to integer before inserting in sqlite3 db which introduces and error of 110 meters at the equator and 80 at 45 lat which seems acceptable on city level

### Invocation

`./build/owmc2sqlite <json file> <sqlite3 file>`

* `<json file>` can be `-` to indicate that the tool should use `stdin` for input
* `<sqlite3 file>` will be deleted at start and will hold the sqlite3 data at the end

### Typical performance figures

For best results you should use a tmpfs mount for the destination file

Current JSON dataset holds nearly 210k records trying to convert them writing on a standard SSD drive will take some 30 **minutes**. Using a tmpfs mount finishes in just 15 **seconds** Current resulting file is just 10MB and can easy fit in the RAM of any modern system.

```
$ time ./build/owmc2sqlite city.list.json ./sample.sqlite3
Table cities in ./sample.sqlite3 created! Let's hope it is in tmpfs or this might take SOME time...
index creation starts
index creation ends
Parsing done! Array items:209579 cities:209579 (6 disabled)  ALLOK!
real    31m43.194s
user    0m17.006s
sys     1m35.166s
$ time ./build/owmc2sqlite city.list.json ./build/sample.sqlite3
Table cities in ./build/sample.sqlite3 created! Let's hope it is in tmpfs or this might take SOME time...
index creation starts
index creation ends
Parsing done! Array items:209579 cities:209579 (6 disabled)  ALLOK!
real    0m14.421s
user    0m4.072s
sys     0m10.221s
```
Issuing SQL queries like `SELECT * FROM cities WHERE lat BETWEEN -10000 AND 10000 AND lon BETWEEN -10000 and 10000` is practically instantaneous no matter if file is on SSD or RAM


### Licenses

This code is under MIT License as is the portion of RapidJSON used here. SQLite is in public domain so you're free to use this tool anyway you like


## ENJOY :)
