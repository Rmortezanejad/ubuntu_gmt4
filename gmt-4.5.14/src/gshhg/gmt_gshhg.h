/*	$Id: gmt_gshhg.h 10289 2014-12-28 21:17:06Z pwessel $
 *
 * Include file defining macros, functions and structures used in gshhg.c
 *
 * Paul Wessel, SOEST
 *
 *	Copyright (c) 1996-2015 by P. Wessel and W. H. F. Smith
 *	See LICENSE.TXT file for copying and redistribution conditions.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 or any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	Contact info: www.soest.hawaii.edu/pwessel
 *
 *	1-JUL-2014.  For use with GSHHG version 2.3.1
 */

#ifndef _GMT_GSHHG
#define _GMT_GSHHG
#define _POSIX_SOURCE 1		/* GSHHG code is POSIX compliant */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "gshhg.h"

#ifdef WIN32
#pragma warning( disable : 4996 )
#endif

#ifndef M_PI
#define M_PI          3.14159265358979323846
#endif

#ifndef SEEK_CUR	/* For really ancient systems */
#define SEEK_CUR 1
#endif

#define GSHHG_PROG_VERSION	"1.18"

/* For byte swapping on little-endian systems (GSHHG is defined to be bigendian) */

#define swabi4(i4) (((i4) >> 24) + (((i4) >> 8) & 65280) + (((i4) & 65280) << 8) + (((i4) & 255) << 24))

#endif	/* _GMT_GSHHG */
