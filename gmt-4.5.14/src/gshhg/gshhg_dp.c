/*	$Id: gshhg_dp.c 10289 2014-12-28 21:17:06Z pwessel $
 *
 *	Copyright (c) 1996-2015 by P. Wessel and W. H. F. Smith
 *	See LICENSE.TXT file for copying and redistribution conditions.
 *
 * gshhg_dp applies the Douglas-Peucker algorithm to simplify a line
 * segment given a tolerance.  The algorithm is based on the paper
 * Douglas, D. H., and T. K. Peucker, Algorithms for the reduction
 *   of the number of points required to represent a digitized line
 *   of its caricature, Can. Cartogr., 10, 112-122, 1973.
 * The implementation of this algorithm has been kindly provided by
 * Dr. Gary J. Robinson, Environmental Systems Science Centre,
 * University of Reading, Reading, UK (gazza@mail.nerc-essc.ac.uk)
 *
 * Paul Wessel, 18-MAY-1999
 * VERSION:	1-JUL-2014.  For use with GSHHG version 2.3.1
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
 */

#include "gmt_gshhg.h"

#define sqr(x) ((x)*(x))
#define D2R (M_PI/180.0)
#define F (D2R * 0.5 * 1.0e-6)
#define FALSE 0
#define VNULL (void *)NULL

void *get_memory (void *prev_addr, int n, size_t size, char *progname);

int main (int argc, char **argv)
{
	FILE	*fp_in = NULL, *fp_out = NULL;
	int	n_id, n_out, n, k, verbose = FALSE, *index = NULL;
	int	n_tot_in, n_tot_out, n_use;
	uint32_t k32;
#ifdef WORDS_BIGENDIAN
	static const int flip = 0;
#else
	static const int flip = 1;
#endif
	int *x = NULL, *y = NULL;
	size_t n_read;
	double	redux, redux2, tolerance = 0.0;
	struct	GSHHG_HEADER h;
	struct	GSHHG_POINT p;

	int Douglas_Peucker_i (int x_source[], int y_source[], int n_source, double band, int index[]);

	if (argc < 2 || !(argc == 4 || argc == 5)) {
		fprintf (stderr, "gshhg_dp %s - Line reduction of GSHHG using the Douglas-Peucker algorithm\n\n", GSHHG_PROG_VERSION);
		fprintf (stderr, "usage:  gshhg_dp input.b tolerance output.b [-v]\n");
		fprintf (stderr, "\ttolerance is maximum mismatch in km\n");
		fprintf (stderr, "\t-v will run in verbose mode and report shrinkage\n");
		exit (EXIT_FAILURE);
	}

	verbose = (argc == 5);
	tolerance = atof (argv[2]);
	if (verbose) fprintf (stderr,"gshhg_dp: Tolerance used is %g km\n", tolerance);
	fp_in  = fopen(argv[1], "rb");
	fp_out = fopen(argv[3], "wb");

	/* Start shrink loop */

	n_id = n_out = n_tot_in = n_tot_out = 0;

	x = (int *) get_memory (VNULL, 1, sizeof (int), "gshhg_dp");
	y = (int *) get_memory (VNULL, 1, sizeof (int), "gshhg_dp");
	index = (int *) get_memory (VNULL, 1, sizeof (int), "gshhg_dp");

	n_read = fread ((void *)&h, sizeof (struct GSHHG_HEADER), (size_t)1, fp_in);

	while (n_read == 1) {

		if (flip) {
			h.id = swabi4 ((unsigned int)h.id);
			h.n  = swabi4 ((unsigned int)h.n);
			h.west  = swabi4 ((unsigned int)h.west);
			h.east  = swabi4 ((unsigned int)h.east);
			h.south = swabi4 ((unsigned int)h.south);
			h.north = swabi4 ((unsigned int)h.north);
			h.area  = swabi4 ((unsigned int)h.area);
			h.flag   = swabi4 ((unsigned int)h.flag);
			h.container  = swabi4 ((unsigned int)h.container);
			h.ancestor  = swabi4 ((unsigned int)h.ancestor);
		}
		/* version = (h.flag >> 8) & 255; */
		if (verbose) fprintf (stderr, "Poly %6d", h.id);	

		x = (int *) get_memory ((void *)x, h.n, sizeof (int), "gshhg_dp");
		y = (int *) get_memory ((void *)y, h.n, sizeof (int), "gshhg_dp");
		index = (int *) get_memory ((void *)index, h.n, sizeof (int), "gshhg_dp");

		for (k32 = 0; k32 < h.n; k32++) {
			if (fread ((void *)&p, sizeof(struct GSHHG_POINT), (size_t)1, fp_in) != 1) {
				fprintf (stderr,"gshhg_dp:  ERROR reading data point.\n");
				exit (EXIT_FAILURE);
			}
			if (flip) {
				p.x = swabi4 ((unsigned int)p.x);
				p.y = swabi4 ((unsigned int)p.y);
			}
			x[k] = p.x;
			y[k] = p.y;
		}
		n_tot_in += h.n;
		
		n_use = (x[0] == x[h.n-1] && y[0] == y[h.n-1]) ? h.n-1 : h.n;

		n = Douglas_Peucker_i (x, y, n_use, tolerance, index);
		
		if (n > 2) {
			index[n] = 0;
			n++;
			redux = 100.0 * (double) n / (double) h.n;
			h.id = n_out;
			h.n = n;
			if (fwrite ((void *)&h, sizeof (struct GSHHG_HEADER), (size_t)1, fp_out) != 1) {
				fprintf(stderr,"gshhg_dp:  ERROR  writing file header.\n");
				exit (EXIT_FAILURE);
			}
			for (k = 0; k < n; k++) {
				p.x = x[index[k]];
				p.y = y[index[k]];
				if (fwrite((void *)&p, sizeof(struct GSHHG_POINT), (size_t)1, fp_out) != 1) {
					fprintf(stderr,"gshhg_dp:  ERROR  writing data point.\n");
					exit (EXIT_FAILURE);
				}
			}
			n_out++;
			n_tot_out += n;
		}
		else
			redux = 0.0;
		if (verbose) fprintf (stderr, "\t%.1f %% retained\n", redux);
		
		n_id++;

		n_read = fread ((void *)&h, sizeof (struct GSHHG_HEADER), (size_t)1, fp_in);
	}
		
	free ((void *)x);	
	free ((void *)y);	
	free ((void *)index);	
		
	fclose (fp_in);
	fclose (fp_out);

	redux = 100.0 * (1.0 - (double) n_tot_out / (double) n_tot_in);
	redux2 = 100.0 * (1.0 - (double) n_out / (double) n_id);
	printf ("gshhg_dp at %g km:\n# of points reduced by %.1f%% (out %d, in %d)\n# of polygons reduced by %.1f%% out (%d, in %d)\n", tolerance, redux, n_tot_out, n_tot_in, redux2, n_out, n_id);

	exit (EXIT_SUCCESS);
}

/* Stack-based Douglas Peucker line simplification routine */
/* returned value is the number of output points */
/* Kindly provided by  Dr. Gary J. Robinson,
 *	Environmental Systems Science Centre,
 *	University of Reading, Reading, UK
 */

int Douglas_Peucker_i (int x_source[], int y_source[], int n_source, double band, int index[])
/* x_source[]: Input coordinates in micro-degrees	*/
/* y_source[]:						*/
/* n_source:	Number of points			*/
/* band:	tolerance in kilometers 		*/
/* index[]:	output co-ordinates indices 		*/
{
	int	n_stack, n_dest, start, end, i, sig;
	int	*sig_start, *sig_end;	/* indices of start&end of working section */

	double dev_sqr, max_dev_sqr, band_sqr;
	double  x12, y12, d12, x13, y13, d13, x23, y23, d23;

        /* check for simple cases */

        if ( n_source < 3 ) return(0);    /* one or two points */

        /* more complex case. initialize stack */

 	sig_start = (int *) get_memory (VNULL, n_source, sizeof (int), "Douglas_Peucker_i");
	sig_end   = (int *) get_memory (VNULL, n_source, sizeof (int), "Douglas_Peucker_i");
	
	band *= 360.0 / (2.0 * M_PI * 6371.007181);	/* Now in degrees */
	band_sqr = sqr(band);
		
	n_dest = 0;

        sig_start[0] = 0;
        sig_end[0] = n_source-1;

        n_stack = 1;

        /* while the stack is not empty  ... */

        while ( n_stack > 0 )
        {
                /* ... pop the top-most entries off the stacks */

                start = sig_start[n_stack-1];
                end = sig_end[n_stack-1];

                n_stack--;

                if ( end - start > 1 )  /* any intermediate points ? */
                {
                        /* ... yes, so find most deviant intermediate point to
                               either side of line joining start & end points */

                x12 = 1.0e-6 * (x_source[end] - x_source[start]);
                if (fabs (x12) > 180.0) x12 = 360.0 - fabs (x12);
                y12 = 1.0e-6 * (y_source[end] - y_source[start]);
		x12 *= cos (F * (y_source[end] + y_source[start]));
		d12 = sqr(x12) + sqr(y12);

                for ( i = start + 1, sig = start, max_dev_sqr = -1.0; i < end; i++ )
                        {
                                x13 = 1.0e-6 * (x_source[i] - x_source[start]);
                                y13 = 1.0e-6 * (y_source[i] - y_source[start]);
				if (fabs (x13) > 180.0) x13 = 360.0 - fabs (x13);
                                x13 *= cos (F * (y_source[i] + y_source[start]));

                                x23 = 1.0e-6 * (x_source[i] - x_source[end]);
                                y23 = 1.0e-6 * (y_source[i] - y_source[end]);
				if (fabs (x23) > 180.0) x23 = 360.0 - fabs (x23);
                                x23 *= cos (F * (y_source[i] + y_source[end]));
                                
                                d13 = sqr(x13) + sqr(y13);
                                d23 = sqr(x23) + sqr(y23);

                                if ( d13 >= ( d12 + d23 ) )
                                        dev_sqr = d23;
                                else if ( d23 >= ( d12 + d13 ) )
                                        dev_sqr = d13;
                                else
                                        dev_sqr =  sqr( x13 * y12 - y13 * x12 ) / d12;

                                if ( dev_sqr > max_dev_sqr  )
                                {
                                        sig = i;
                                        max_dev_sqr = dev_sqr;
                                }
                        }

                        if ( max_dev_sqr < band_sqr )   /* is there a sig. intermediate point ? */
                        {
                                /* ... no, so transfer current start point */

                                index[n_dest] = start;
                                n_dest++;
                        }
                        else
                        {
                                /* ... yes, so push two sub-sections on stack for further processing */

                                n_stack++;

                                sig_start[n_stack-1] = sig;
                                sig_end[n_stack-1] = end;

                                n_stack++;

                                sig_start[n_stack-1] = start;
                                sig_end[n_stack-1] = sig;
                        }
                }
                else
                {
                        /* ... no intermediate points, so transfer current start point */

                        index[n_dest] = start;
                        n_dest++;
                }
        }


        /* transfer last point */

        index[n_dest] = n_source-1;
        n_dest++;

	free ((void *)sig_start);
	free ((void *)sig_end);
	
        return (n_dest);
}

void *get_memory (void *prev_addr, int n, size_t size, char *progname)
{
        void *tmp;

	if (n == 0) return(VNULL); /* Take care of n = 0 */

	if (prev_addr) {
		if ((tmp = realloc ((void *) prev_addr, (size_t) (n * size))) == VNULL) {
			fprintf (stderr, "gshhg_dp Fatal Error: %s could not reallocate more memory, n = %d\n", progname, n);
			exit (EXIT_FAILURE);
		}
	}
	else {
		if ((tmp = calloc ((size_t) n, (size_t) size)) == VNULL) {
			fprintf (stderr, "gshhg_dp Fatal Error: %s could not allocate memory, n = %d\n", progname, n);
			exit (EXIT_FAILURE);
		}
	}
	return (tmp);
}
