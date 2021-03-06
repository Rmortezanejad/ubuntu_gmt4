/*	$Id: gmt_mgg_header2.c 9545 2011-07-27 19:31:54Z pwessel $
 *
 *	Code donated by David Divens, NOAA/NGDC
 *	Distributed under the GNU Public License (see LICENSE.TXT for details)
 *--------------------------------------------------------------------*/

/* Comments added by P. Wessel:
 *
 * 1) GRD98 can now support pixel grids since 11/24/2006.
 * 2) Learned that 1/x_inc and 1/y_inc are stored as integers.  This means
 *    GRD98 imposes restrictions on x_inc & y_inc.
 */

#include "gmt_mgg_header2.h"

#define MIN_PER_DEG		60.0
#define SEC_PER_MIN		60.0
#define SEC_PER_DEG		(SEC_PER_MIN * MIN_PER_DEG)
#define BYTE_SIZE

int swap_header (MGG_GRID_HEADER_2 *header);

static double dms2degrees(int deg, int min, int sec)
{
    double  decDeg = (double)deg;

    decDeg += (double)min / MIN_PER_DEG;
    decDeg += (double)sec / SEC_PER_DEG;

    return decDeg;
}

static void degrees2dms(double degrees, int *deg, int *min, int *sec)
{
	/* Round off to the nearest half second */
	if (degrees < 0) degrees -= (0.5 / SEC_PER_DEG);

	*deg = (int)degrees;
	degrees -= *deg;

	degrees *= MIN_PER_DEG;
	*min = (int)(degrees);
	degrees -= *min;

	*sec = (int)(degrees * SEC_PER_MIN);
}

int GMT2MGG2(struct GRD_HEADER *gmt, MGG_GRID_HEADER_2 *mgg)
{
	double f;
	memset(mgg, 0, sizeof(MGG_GRID_HEADER_2));
	
	mgg->version     = MGG_MAGIC_NUM + MGG_VERSION;
	mgg->length      = sizeof(MGG_GRID_HEADER_2);
	mgg->dataType    = 1;
	
	mgg->cellRegistration = gmt->node_offset;
	mgg->lonNumCells = gmt->nx;
	f  = gmt->x_inc * SEC_PER_DEG;
	mgg->lonSpacing  = (int)rint(f);
	if (fabs (f - (double)mgg->lonSpacing) > GMT_CONV_LIMIT) return (GMT_GRDIO_GRD98_XINC);
	degrees2dms(gmt->x_min, &mgg->lonDeg, &mgg->lonMin, &mgg->lonSec);
	
	mgg->latNumCells = gmt->ny;
	f  = gmt->y_inc * SEC_PER_DEG;
	mgg->latSpacing  = (int)rint(gmt->y_inc * SEC_PER_DEG);
	if (fabs (f - (double)mgg->latSpacing) > GMT_CONV_LIMIT) return (GMT_GRDIO_GRD98_YINC);
	degrees2dms(gmt->y_max, &mgg->latDeg, &mgg->latMin, &mgg->latSec);

	/* Default values */
	mgg->gridRadius  = -1;
	mgg->precision   = DEFAULT_PREC;
	mgg->nanValue    = MGG_NAN_VALUE;
	mgg->numType     = sizeof(int);
	mgg->minValue    = (int)rint(gmt->z_min * mgg->precision);
	mgg->maxValue    = (int)rint(gmt->z_max * mgg->precision);

	/* Data fits in two byte boundry */
	if ((-SHRT_MAX <= mgg->minValue) && (mgg->maxValue <= SHRT_MAX)) {
		mgg->numType = sizeof(short);
		mgg->nanValue = (short)SHRT_MIN;
	}
#ifdef BYTE_SIZE
	/* Data fits in one byte boundry */
	if ((gmt->z_min >= 0) && (gmt->z_max <= 127)) {
		mgg->numType   = sizeof(char);
		mgg->nanValue  = (char)255;
		mgg->precision = 1;
		mgg->minValue  = (int)gmt->z_min;
		mgg->maxValue  = (int)gmt->z_max;
	}
#endif
	return (GMT_NOERROR);
}

static void MGG2_2GMT(MGG_GRID_HEADER_2 *mgg, struct GRD_HEADER *gmt)
{
	int one_or_zero;
	
	/* Do not memset the gmt header since it has the file name set */
	
	gmt->type = GMT_grd_format_decoder ("rf");
	gmt->node_offset    = mgg->cellRegistration;
	one_or_zero	    = 1 - gmt->node_offset;
	gmt->nx             = mgg->lonNumCells;
	gmt->x_min          = dms2degrees(mgg->lonDeg, mgg->lonMin, mgg->lonSec);
	gmt->x_inc          = dms2degrees(0, 0, mgg->lonSpacing);
	gmt->x_max          = gmt->x_min + (gmt->x_inc * (gmt->nx - one_or_zero));

	gmt->ny             = mgg->latNumCells;
	gmt->y_max          = dms2degrees(mgg->latDeg, mgg->latMin, mgg->latSec);
	gmt->y_inc          = dms2degrees(0, 0, mgg->latSpacing);
	gmt->y_min          = gmt->y_max - (gmt->y_inc * (gmt->ny - one_or_zero));
 
	gmt->z_min          = (double)mgg->minValue / (double)mgg->precision;
	gmt->z_max          = (double)mgg->maxValue / (double)mgg->precision;
	gmt->z_scale_factor = 1.0;
	gmt->z_add_offset   = 0;
}

static void swap_word(void* ptr)
{
   unsigned char *tmp = ptr;
   unsigned char a = tmp[0];
   tmp[0] = tmp[1];
   tmp[1] = a;
}

static void swap_long (void *ptr)
{
   unsigned char *tmp = ptr;
   unsigned char a = tmp[0];
   tmp[0] = tmp[3];
   tmp[3] = a;

   a = tmp[1];
   tmp[1] = tmp[2];
   tmp[2] = a;
}

int swap_header (MGG_GRID_HEADER_2 *header)
{
	int i, version;
	/* Determine if swapping is needed */
	if (header->version == (MGG_MAGIC_NUM + MGG_VERSION)) return (0);	/* Version matches, No need to swap */
	version = header->version;
	swap_long (&version);
	if (version != (MGG_MAGIC_NUM + MGG_VERSION)) return (-1);		/* Cannot make sense of header */
	/* Here we come when we do need to swap */
	swap_long (&header->version);
	swap_long (&header->length);
	swap_long (&header->dataType);
	swap_long (&header->latDeg);
	swap_long (&header->latMin);
	swap_long (&header->latSec);
	swap_long (&header->latSpacing);
	swap_long (&header->latNumCells);
	swap_long (&header->lonDeg);
	swap_long (&header->lonMin);
	swap_long (&header->lonSec);
	swap_long (&header->lonSpacing);
	swap_long (&header->lonNumCells);
	swap_long (&header->minValue);
	swap_long (&header->maxValue);
	swap_long (&header->gridRadius);
	swap_long (&header->precision);
	swap_long (&header->nanValue);
	swap_long (&header->numType);
	swap_long (&header->waterDatum);
	swap_long (&header->dataLimit);
	swap_long (&header->cellRegistration);
	for (i = 0; i < GRD98_N_UNUSED; i++) swap_long (&header->unused[i]);
	return (1);	/* Signal we need to swap the data also */
}

GMT_LONG GMT_is_mgg2_grid (struct GRD_HEADER *header)
{	/* Determine if file is a GRD98 file */
	FILE *fp = NULL;
	MGG_GRID_HEADER_2 mggHeader;
	int ok;

	if (!strcmp(header->name, "=")) return (GMT_GRDIO_PIPE_CODECHECK);	/* Cannot check on pipes */
	if ((fp = GMT_fopen(header->name, GMT_io.r_mode)) == NULL) return (GMT_GRDIO_OPEN_FAILED);

	memset (&mggHeader, 0, sizeof(MGG_GRID_HEADER_2));
	if (GMT_fread(&mggHeader, sizeof(MGG_GRID_HEADER_2), (size_t)1, fp) != 1) return (GMT_GRDIO_READ_FAILED);

	/* Swap header bytes if necessary; ok is 0|1 if successful and -1 if bad file */
	ok = swap_header (&mggHeader);

	/* Check the magic number and size of header */
	if (ok == -1) return (-1);	/* Not this kind of file */
	header->type = GMT_grd_format_decoder ("rf");
	return (header->type);
}

GMT_LONG mgg2_read_grd_info (struct GRD_HEADER *header)
{
	FILE			*fp = NULL;
	MGG_GRID_HEADER_2	mggHeader;
	int ok;

	if (!strcmp(header->name, "=")) {
		fp = GMT_stdin;
	} else if ((fp = GMT_fopen(header->name, GMT_io.r_mode)) == NULL)
		return (GMT_GRDIO_OPEN_FAILED);

	memset ((void *)&mggHeader, 0, sizeof(MGG_GRID_HEADER_2));
	if (GMT_fread(&mggHeader, sizeof(MGG_GRID_HEADER_2), (size_t)1, fp) != 1) return (GMT_GRDIO_READ_FAILED);

	/* Swap header bytes if necessary; ok is 0|1 if successful and -1 if bad file */
	ok = swap_header(&mggHeader);

	/* Check the magic number and size of header */
	if (ok == -1) {
		fprintf(stderr, "GMT Fatal Error: Unrecognized header, expected 0x%04X saw 0x%04X\n", MGG_MAGIC_NUM + MGG_VERSION, mggHeader.version);
		return (GMT_GRDIO_GRD98_BADMAGIC);
	}

	if (mggHeader.length != sizeof(MGG_GRID_HEADER_2)) {
		fprintf(stderr, "GMT Fatal Error: Invalid grid header size, expected %d, found %d\n", (int)sizeof(MGG_GRID_HEADER_2), mggHeader.length);
		return (GMT_GRDIO_GRD98_BADLENGTH);
	}

	if (fp != GMT_stdin) GMT_fclose(fp);
	
	MGG2_2GMT(&mggHeader, header);
	
	return (GMT_NOERROR);
}

GMT_LONG mgg2_write_grd_info (struct GRD_HEADER *header)
{
	FILE			*fp = NULL;
	MGG_GRID_HEADER_2	mggHeader;
	GMT_LONG err;
	
	if (!strcmp(header->name, "=")) {
		fp = GMT_stdout;
	} else if ((fp = GMT_fopen(header->name, GMT_io.w_mode)) == NULL)
		return (GMT_GRDIO_CREATE_FAILED);
	
	if ((err = GMT2MGG2(header, &mggHeader))) return (err);

	if (GMT_fwrite (&mggHeader, sizeof(MGG_GRID_HEADER_2), (size_t)1, fp) != 1) return (GMT_GRDIO_WRITE_FAILED);

	if (fp != GMT_stdout) GMT_fclose (fp);
	
	return (GMT_NOERROR);
}

GMT_LONG mgg2_read_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, GMT_LONG pad[], GMT_LONG complex)
{
	MGG_GRID_HEADER_2	mggHeader;
	FILE  *fp     = NULL;
	int  *tLong  = NULL;
	short *tShort = NULL;
	char  *tChar  = NULL;
	float *tFloat = NULL;
	GMT_LONG first_col, last_col, first_row, last_row;
	GMT_LONG j, j2, width_in, height_in, i_0_out, inc = 1;
	GMT_LONG i, kk, ij, width_out;
	GMT_LONG piping = FALSE, swap_all = FALSE, is_float = FALSE;
	GMT_LONG *k = NULL;			/* Array with indices */
	long long_offset;	/* For fseek only */
	
	GMT_err_pass (GMT_grd_prep_io (header, &w, &e, &s, &n, &width_in, &height_in, &first_col, &last_col, &first_row, &last_row, &k), header->name);

	memset ((void *)&mggHeader, 0, sizeof(MGG_GRID_HEADER_2));
	if (!strcmp (header->name, "=")) {
		fp = GMT_stdin;
		piping = TRUE;
	}
	else if ((fp = GMT_fopen (header->name, GMT_io.r_mode)) != NULL) {
		if (GMT_fread(&mggHeader, sizeof(MGG_GRID_HEADER_2), (size_t)1, fp) != 1) return (GMT_GRDIO_READ_FAILED);
		swap_all = swap_header(&mggHeader);
		if (swap_all == -1) return (GMT_GRDIO_GRD98_BADMAGIC);
		if (mggHeader.numType == 0) mggHeader.numType = sizeof(int);
	}

	else {
		return (GMT_GRDIO_OPEN_FAILED);
	}
	
	is_float = (mggHeader.numType < 0 && abs(mggHeader.numType) == (int)sizeof(float));	/* Float file */
	
	width_out = (GMT_LONG)width_in;		/* Width of output array */
	if (pad[0] > 0) width_out += pad[0];
	if (pad[1] > 0) width_out += pad[1];
	
	i_0_out = pad[0];		/* Edge offset in output */
	if (complex) {	/* Need twice as much output space since we load every 2nd cell */
		width_out *= 2;
		i_0_out *= 2;
		inc = 2;
	}
	
	tLong  = (int *) GMT_memory (CNULL, (size_t)header->nx, sizeof (int), "mgg_read_grd");
	tShort = (short *)tLong;
	tChar  = (char *)tLong;
	tFloat  = (float *)tLong;

	if (piping)	{ /* Skip data by reading it */
		for (j = 0; j < first_row; j++) if (GMT_fread ((void *) tLong, (size_t)abs(mggHeader.numType), (size_t)header->nx, fp) != (size_t)header->nx) return (GMT_GRDIO_READ_FAILED);
	} else { /* Simply seek by it */
		long_offset = (long)(first_row * header->nx * abs(mggHeader.numType));
		if (GMT_fseek (fp, long_offset, 1)) return (GMT_GRDIO_SEEK_FAILED);
	}
	
	for (j = first_row, j2 = 0; j <= last_row; j++, j2++) {
		if (GMT_fread ((void *) tLong, (size_t)abs(mggHeader.numType), (size_t)header->nx, fp) != (size_t)header->nx) return (GMT_GRDIO_READ_FAILED);
		
		ij = (j2 + pad[3]) * width_out + i_0_out;	/* Already has factor of 2 in it if complex */
		for (i = 0; i < width_in; i++) {
			kk = ij + i*inc;
			if (mggHeader.numType == sizeof(int)) {
				if (swap_all) swap_long (&tLong[k[i]]);
				if (tLong[k[i]] == mggHeader.nanValue) grid[kk] = GMT_f_NaN;
				else grid[kk] = (float) tLong[k[i]] / (float) mggHeader.precision;
			}
			else if (is_float) {
				if (swap_all) swap_long (&tLong[k[i]]);
				if (tLong[k[i]] == mggHeader.nanValue) grid[kk] = GMT_f_NaN;
				else grid[kk] = tFloat[k[i]];
			}
			
			else if (mggHeader.numType == sizeof(short)) {
				if (swap_all) swap_word(&tShort[k[i]]);
				if (tShort[k[i]] == mggHeader.nanValue) grid[kk] = GMT_f_NaN;
				else grid[kk] = (float) tShort[k[i]] / (float) mggHeader.precision;;
			}
			
			else if (mggHeader.numType == sizeof(char)) {
				if (tChar[k[i]] == mggHeader.nanValue) grid[kk] = GMT_f_NaN;
				else grid[kk] = (float) tChar[k[i]] / (float) mggHeader.precision;;
			}

			else {
				return (GMT_GRDIO_UNKNOWN_TYPE);
			}
		}
	}
	if (piping)	{ /* Skip data by reading it */
		for (j = last_row + 1; j < header->ny; j++) {
			if (GMT_fread ((void *) tLong, (size_t)abs(mggHeader.numType), (size_t)header->nx, fp) != (size_t)header->nx) return (GMT_GRDIO_READ_FAILED);
		}
	}
		
	GMT_free ((void *)k);

	header->nx = (int)width_in;
	header->ny = (int)height_in;
	header->x_min = w;
	header->x_max = e;
	header->y_min = s;
	header->y_max = n;

	header->z_min = DBL_MAX;	header->z_max = -DBL_MAX;
	for (j = 0; j < header->ny; j++) {
		for (i = 0; i < header->nx; i++) {
			ij = (j + pad[3]) * width_out + inc * (i + pad[0]);
			if (GMT_is_fnan (grid[ij])) continue;
			header->z_min = MIN (header->z_min, grid[ij]);
			header->z_max = MAX (header->z_max, grid[ij]);
		}
	}
	if (fp != stdin) GMT_fclose (fp);
	
	GMT_free ((void *)tLong);
	return (GMT_NOERROR);
}

	
GMT_LONG mgg2_write_grd (struct GRD_HEADER *header, float *grid, double w, double e, double s, double n, GMT_LONG *pad, GMT_LONG complex)
{
	MGG_GRID_HEADER_2	mggHeader;
	GMT_LONG *k = NULL;			/* Array with indices */
	GMT_LONG i2, kk, err, inc = 1;
	GMT_LONG j, j2, width_out, height_out;
	GMT_LONG first_col, last_col, first_row, last_row;
	GMT_LONG i, ij, width_in, is_float = FALSE;
	
	int  *tLong = NULL;
	short *tShort = NULL;
	char  *tChar = NULL;
	float *tFloat = NULL;
	FILE *fp = NULL;
	
	if (!strcmp (header->name, "=")) {
		fp = GMT_stdout;
	}
	else if ((fp = GMT_fopen (header->name, GMT_io.w_mode)) == NULL)
		return (GMT_GRDIO_CREATE_FAILED);
	
	GMT_err_pass (GMT_grd_prep_io (header, &w, &e, &s, &n, &width_out, &height_out, &first_col, &last_col, &first_row, &last_row, &k), header->name);

	width_in = (GMT_LONG)width_out;		/* Physical width of input array */
	if (pad[0] > 0) width_in += pad[0];
	if (pad[1] > 0) width_in += pad[1];
	if (complex) inc = 2;
	
	header->x_min = w;
	header->x_max = e;
	header->y_min = s;
	header->y_max = n;
	
	/* Find xmin/zmax */
	
	header->z_min = DBL_MAX;	header->z_max = -DBL_MAX;
	for (j = first_row, j2 = pad[3]; j <= last_row; j++, j2++) {
		for (i = first_col, i2 = pad[0]; i <= last_col; i++, i2++) {
			ij = inc * (j2 * width_in + i2);
			if (GMT_is_fnan (grid[ij])) continue;
			header->z_min = MIN (header->z_min, grid[ij]);
			header->z_max = MAX (header->z_max, grid[ij]);
		}
	}
	
	/* store header information and array */
	if ((err = GMT2MGG2(header, &mggHeader))) return (err);;
	if (GMT_fwrite (&mggHeader, sizeof (MGG_GRID_HEADER_2), (size_t)1, fp) != 1) return (GMT_GRDIO_WRITE_FAILED);
	is_float = (mggHeader.numType < 0 && abs(mggHeader.numType) == (int)sizeof(float));	/* Float file */

	tLong  = (int *) GMT_memory (CNULL, (size_t)width_in, sizeof (int), "mgg_write_grd");
	tShort = (short *) tLong;
	tChar  = (char *) tLong;
	tFloat  = (float *) tLong;
	
	i2 = first_col + pad[0];
	for (j = 0, j2 = first_row + pad[3]; j < height_out; j++, j2++) {
		ij = j2 * width_in + i2;
		for (i = 0; i < width_out; i++) {
			kk = inc * (ij+k[i]);
			if (GMT_is_fnan (grid[kk])) {
				if (mggHeader.numType == sizeof(int))       tLong[i]  = mggHeader.nanValue;
				else if (is_float) tFloat[i]  = (float)mggHeader.nanValue;
				else if (mggHeader.numType == sizeof(short)) tShort[i] = (short)mggHeader.nanValue;
				else if (mggHeader.numType == sizeof(char))  tChar[i] = (char)mggHeader.nanValue;
				else {
					return (GMT_GRDIO_UNKNOWN_TYPE);
				}
			} else {
				if (grid[kk] > -0.1 && grid[kk] < 0) grid[kk] = (float)(-0.1);

				if (mggHeader.numType == sizeof(int)) {
					tLong[i] = (int)rint ((double)grid[kk] * mggHeader.precision);
				}
				else if (is_float) {
					tFloat[i] = grid[kk];
				}
				
				else if (mggHeader.numType == sizeof(short)) {
					tShort[i] = (short) rint((double)grid[kk] * mggHeader.precision);
				}
				
				else if (mggHeader.numType == sizeof(char)) {
					tChar[i] = (char) rint((double)grid[kk] * mggHeader.precision);
				}
				
				else {
					return (GMT_GRDIO_UNKNOWN_TYPE);
				}
			}
		}
		if (GMT_fwrite ((void *)tLong, (size_t)abs(mggHeader.numType), (size_t)width_out, fp) != (size_t)width_out) return (GMT_GRDIO_WRITE_FAILED);
	}
	GMT_free ((void *)k);
	if (fp != GMT_stdout) GMT_fclose (fp);
	
	GMT_free ((void *)tLong);
	
	return (GMT_NOERROR);
}
