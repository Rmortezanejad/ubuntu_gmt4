/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *	$Id: x_init.c 9545 2011-07-27 19:31:54Z pwessel $
 *
 * XINIT will create the xx_base.b and xx_legs.b files and write out
 * the first header record (in xx_base.b) that tells which record number
 * to use next time, i.e. 1 in our case.
 *
 * Author:	Paul Wessel
 * Date:	18-JAN-1987
 * Last rev:	07-MAR-2000	POSIX
 *
 */

#include "gmt.h"
#include "x_system.h"

int main (int argc, char **argv)
{
	char buffer[REC_SIZE];
	GMT_LONG i;
	FILE *fhandle = NULL, *fp = NULL;

	if (!(argc == 2 && argv[1][1] == 'I')) {
		fprintf (stderr, "xinit - Initialization of new xover databases\n\n");
		fprintf (stderr, "usage: xinit -I\n");
		exit (EXIT_FAILURE);
	}

	for (i = 0; i < REC_SIZE; i++) buffer[i] = ' ';

	/* Create the xover data base */

	if ((fhandle = fopen ("xx_base.b","wb")) == NULL) {
		fprintf (stderr,"xinit : Could not create xx_base.b\n");
		exit (EXIT_FAILURE);
	}
	else
		fprintf(stderr,"xinit : Successfully created xx_base.b\n");

	sprintf (buffer,"%10ld xx_base.b header",1L);
	if ((i = fwrite ((void *)buffer, (size_t)REC_SIZE, (size_t)1, fhandle)) != 1) {
		fprintf (stderr,"write error on xx_base.b!");
		exit (EXIT_FAILURE);
	}
	else
		fprintf (stderr,"xinit : Successfully initialized xx_base.b\n");

	fclose (fhandle);

	/* Create the legs file */

	if ((fp = fopen ("xx_legs.b", "w")) == NULL) {
		fprintf (stderr,"xinit : Could not create xx_legs.b\n");
		exit (EXIT_FAILURE);
	}
	else
		fprintf (stderr,"xinit : Successfully created xx_legs.b\n");

	fclose (fp);

	printf ("xinit: done!\n");

	exit (EXIT_SUCCESS);

}

