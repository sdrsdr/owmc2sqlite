#*********
# Copyright (c) 2021 Stoian Ivanov 
#  This file is under a MIT License
#**********

#Octave/Matlab sript to calculate distance per lon angle at given lat
#https://cocalc.com/ is a great way to use Octave without installing it 

R = 6371e3/1000;  #radius of spherical earth in KM
lon_km=100; #desired distence at surface for calculated lon angle at given lat
tbl=[];
for i=0:8
 deg4lon_km=rad2deg((lon_km+60*sin(deg2rad(i*10)))/(R*cos(deg2rad(i*10))));
 if (i>=7)
   deg4lon_km+=0.6;
 endif
 tbl=[tbl; i*10,deg4lon_km];
endfor 
tbl

#check errors at "square" boundaries
errs=[];
for i=0:7
degLow=tbl(i+1,2);
atLow=deg2rad(degLow)*R*cos(deg2rad(i*10));
atHigh=deg2rad(degLow)*R*cos(deg2rad((i+1)*10));
atMid=deg2rad(degLow)*R*cos(deg2rad(i*10+5));
errs=[errs; i*10, atLow,lon_km-atLow,atMid,(i+1)*10,atHigh, lon_km-atHigh];
endfor
disp("     lowLat     len@low        err     len@mind     highLat    len@high       err")
errs