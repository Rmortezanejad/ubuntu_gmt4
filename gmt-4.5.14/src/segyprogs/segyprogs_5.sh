#!/bin/sh
#
#	$Id: segyprogs_5.sh 10335 2015-10-31 21:15:28Z pwessel $
#
# script to plot mendo wa1 combined data
#
# cdp = 40 * coordinate on line, 30km max depth
#

area1=-R-35/6/0/30
proj1="-Jx0.15i/-0.15i"
outfile=segyprogs_5.ps

makecpt -T-5/5/1 -Z -Cpolar > test.cpt
segy2grd $area1  -Sc -X0.1 -Y0.1 -I0.5/0.2 wa1_mig13.segy -Gtest.nc -V
grdimage $area1 $proj1 -K test.nc -Ctest.cpt -Xc -P > $outfile
psbasemap -R -J -Bf5a10 -O >> $outfile
rm -f test.nc test.cpt
