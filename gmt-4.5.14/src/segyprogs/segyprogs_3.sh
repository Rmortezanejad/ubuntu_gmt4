#!/bin/sh
#	$Id: segyprogs_3.sh 10333 2015-10-31 02:30:06Z pwessel $

# script to plot mendo wa1 combined data
#
# cdp = 40 * coordinate on line, 30km max depth
#
area1=-R-35/6/-35/6/0/30
proj1="-Jx0.15i/-0.15i -Jz-0.15i -E175/05"
outfile=segyprogs_3.ps

psxyz $area1 $proj1 -K /dev/null -Xc -P > $outfile
pssegyz wa1_mig13.segy -R $proj1 -X0.1i -D0.35 -C8.0 -Y0.1i -W -F0/255/0 -B-1.2 -Sc/5 -O -K >> $outfile
pssegyz wa1_mig13.segy -R $proj1 -X0.1i -D0/0.35 -C8.0 -Y0.1i -F255/0/0 -B-1.2 -S5/c -O -K >> $outfile
psbasemap -R $proj1 -Bf5a10:X:/f5a10:Y:/f5a10Z -O >> $outfile
