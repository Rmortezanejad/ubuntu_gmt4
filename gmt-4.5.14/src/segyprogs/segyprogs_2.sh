#!/bin/sh
#
#	$Id: segyprogs_2.sh 10334 2015-10-31 02:39:08Z pwessel $
#
# script to plot mendo wa1 combined data
#
# cdp = 40 * coordinate on line, 30km max depth
#

area1=-R-35/6/0/30
proj1="-Jx0.15i/-0.15i"
outfile=segyprogs_2.ps

psxy $area1 $proj1 -K -P -Xc -T > $outfile
pssegy wa1_mig13.segy -R -J -X0.1i -D0.35 -C8.0 -Y0.1i -W -F0/255/0 -B-1.2 -Sc -O -K -Ttest.list -V -Z >> $outfile
psbasemap -R -J -Bf5a10 -O >> $outfile
