/* -------------------------------------------------------------------
 *	$Id: mgd77sniffer.c 10289 2014-12-28 21:17:06Z pwessel $
 *      See LICENSE.TXT file for copying and redistribution conditions.
 *
 *    Copyright (c) 2004-2015 by P. Wessel and M. T. Chandler
 *	File:	mgd77sniffer.c
 *
 *	mgd77sniffer scans MGD77 files for errors in three ways: one, point-
 *	by-point scanning to determine if values are within reasonable limits;
 *	two, along-track scanning to find excessive changes in values; three,
 *	comparing ship gathered gravity and topography data with global
 *	reference grids for errors. Errors are reported to standard output
 *	by default with optional output of structured "E77" errata tables.
 *
 *	Authors:
 *		Michael Chandler and Paul Wessel
 *		School of Ocean and Earth Science and Technology
 *		University of Hawaii
 *
 *	Date:	March 2006
 *
 * ------------------------------------------------------------------*/

#include "mgd77.h"
#include "mgd77sniffer.h"

/*
#define HISTOGRAM_MODE 0
#define FIX 0
#define OUTPUT_TEST 0
#define DUMP_DECIMATE 0
*/

#if OUTPUT_TEST == 1
#define OR_TRUE || 1
#define AND_FALSE && 0
#else
#define OR_TRUE
#define AND_FALSE
#endif

#define POS 1
#define NEG 0

int main (int argc, char **argv) {

	/* THE FOLLOWING VARIABLES DO NOT VARY FOR EACH CRUISE */
	int argno, n_cruises = 0, n_grids = 0, n_out_columns, n_paths;
	int interpolant = BCR_BICUBIC, dtc_index = 0;
	unsigned int MGD77_this_bit[32], n_types[N_ERROR_CLASSES], n_bad_sections = 0;
	GMT_LONG pos = 0;

	double time_factor = 1.0, distance_factor = 1.0, maxTime, west=0.0, east=0.0, north=0.0, south=0.0, adjustDC[32];
	double test_slope[5] = {0.1, 10.0, MGD77_METERS_PER_FATHOM, MGD77_FATHOMS_PER_METER}, adjustScale[32];
	double max_speed, min_speed, MGD77_NaN, maxSlope[MGD77_N_NUMBER_FIELDS], maxGap, threshold = 1.0;
	double percent_limit, sim_m[8], sim_b[8], nav_on_land_threshold;
	float *f[8];
	time_t clock;

	char c, tmp_min[16], tmp_max[16], tmp_maxSlope[16], tmp_area[16], *derivative = NULL;
	char *custom_limit_file = NULL, custom_limit_line[BUFSIZ], arguments[BUFSIZ], buffer[BUFSIZ];
	char field_abbrev[8], *speed_units = "m/s", *distance_units = "km";
	char *display = CNULL, fpercent_limit[8], **list = NULL;

	GMT_LONG error = FALSE, nautical = FALSE, custom_max_speed = FALSE, simulate = FALSE;
	GMT_LONG bad_sections = FALSE, custom_min_speed = FALSE, do_regression = TRUE, dist_to_coast = FALSE;
	GMT_LONG custom_warn = FALSE, warn[MGD77_N_WARN_TYPES], custom_maxGap = FALSE, report_raw = FALSE;
	GMT_LONG decimateData = TRUE, forced = FALSE, adjustData = FALSE, flip_flags = FALSE;

	FILE *custom_fp = NULL, *fpout = NULL;

	struct MGD77_SNIFFER_DEFAULTS mgd77snifferdefs[MGD77_N_DATA_FIELDS] = {
#include "mgd77snifferdefaults.h"
	};

	struct BAD_SECTION BadSection[MAX_BAD_SECTIONS];
	
	/* THESE VARIABLES VARY FOR EACH CRUISE AND REQUIRE EXTRA CARE (RESET FOR EACH CRUISE) */
	int type, field, bccCode, col, distanceErrorCount, duplicates[MGD77_N_NUMBER_FIELDS], *iMaxDiff = NULL, n_bad, grav_formula;
	int noTimeCount, noTimeStart, timeErrorCount, timeErrorStart, distanceErrorStart, overLandStart, overLandCount, last_day, utc_offset;
	GMT_LONG i, j, k, m, curr = 0, nwords, nout, nvalues, n_nan, n, npts = 0, *offsetStart = NULL, rec = 0, n_alloc = GMT_CHUNK, n_wrap, extreme;
	unsigned int lowPrecision, lowPrecision5, MGD77_sign_bit[32];

	double gradient, dvalue, dt, ds, **out = NULL, thisArea, speed, prev_speed, **G = NULL, min, *distance = NULL, date, range, range2;
	double *offsetArea = NULL, stat[MGD77_N_STATS], stat2[MGD77_N_STATS], *ship_val = NULL, *grid_val = NULL, max;
	double thisLon, thisLat, lastLon, lastLat, *MaxDiff = NULL, **diff = NULL, *decimated_orig, wrapsum, tcrit, se,  n_days;
	double *offsetLength = NULL, *decimated_new = NULL, recommended_scale, *new_anom = NULL, *old_anom = NULL, IGRF[8], lastCorr = 0.0;

	char timeStr[32], placeStr[128], errorStr[128], outfile[32], abbrev[8], fstat[MGD77_N_STATS][GMT_TEXT_LEN], text[GMT_TEXT_LEN];

	GMT_LONG gotTime, landcruise, *offsetSign = NULL, newScale = FALSE, mtf1, nav_error, spike_amplitude;
	GMT_LONG *prevOffsetSign = NULL, prevFlag, prevType, decimated = FALSE;
#ifdef FIX
	GMT_LONG deleteRecord = FALSE;
#endif

	/* INITIALIZE MEMORY FOR MGD77 DATA STRUCTURES */
	struct MGD77_DATA_RECORD *D = NULL;
	struct MGD77_HEADER H;
	struct MGD77_CONTROL M, Out;
	struct MGD77_GRID_INFO this_grid[8];
	struct MGD77_ERROR *E = NULL;
	struct tm *systemTime = NULL;
	struct MGD77_CARTER C;
	struct GMT_gcal cal;
	
	/* INITIALIZE GMT & MGD77 MACHINERY */
	argc = (int)GMT_begin (argc, argv);
#ifdef DEBUG
	GMT_memtrack_off (GMT_mem_keeper);
#endif
	strncpy (gmtdefs.output_clock_format, "hh:mm:ss.xx", (size_t)GMT_TEXT_LEN);
	GMT_clock_C_format (gmtdefs.output_clock_format, &GMT_io.clock_output, 1);
	MGD77_Init (&M);
	MGD77_Init (&Out);
	clock = time (NULL);
	maxTime = (double)clock;
	systemTime = gmtime (&clock);
	MGD77_carter_init (&C);
	Out.fp = GMT_stdout;
	GMT_make_dnan (MGD77_NaN);
	
	/* INITIALIZE E77 */
	n_types[E77_NAV] = N_NAV_TYPES;
	n_types[E77_VALUE] = N_DEFAULT_TYPES;
	n_types[E77_SLOPE] = N_DEFAULT_TYPES;

	/* TURN ON MGD77SNIFFER ERROR MESSAGES */
	for (i = 0; i<MGD77_N_WARN_TYPES; i++) warn[i] = TRUE;
	
	/* SET PROGRAM DEFAULTS */
	arguments[0] = 0;
	for (i = 0; i < MGD77_N_DATA_FIELDS; i++) MGD77_this_bit[i] = 1 << i;
	maxGap = (double) MGD77_MAX_DS;	/* 5 km by default */
	n_out_columns = 0;			/* No formatted output */
	M.verbose_level = 3;	/* 1 = warnings, 2 = errors, 3 = both */
	M.verbose_dest = 1;		/* 1 = stdout, 2 = stderr */
	max_speed = MGD77_MAX_SPEED;
	min_speed = MGD77_MIN_SPEED;
	derivative = "SPACE";
	display = "";
	percent_limit = MGD77_NaN;
	nav_on_land_threshold = MGD77_DIST_FROM_COAST;
	for (i = 0; i<MGD77_N_NUMBER_FIELDS; i++) {
		maxSlope[i] = mgd77snifferdefs[i].maxSpaceGrad; /* Use spatial gradients by default */
		adjustScale[i] = MGD77_NaN;
		adjustDC[i] = MGD77_NaN;
	}
	memset ((void *)this_grid, 0, 8 * sizeof (struct MGD77_GRID_INFO));

	/* READ COMMAND LINE ARGUMENTS */
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			sprintf(arguments,"%s %s",arguments,argv[i]);
			switch (argv[i][1]) {
				case 'b':
				case 'V':
				case 'R':
				case '\0':
					error += GMT_parse_common_options (argv[i], &west, &east, &south, &north);
					break;
				case 'A':	/* adjust slope and intercept */
					if (!error && sscanf (&argv[i][2], "%[^,]", abbrev) != 1) {
						fprintf (stderr, "%s: SYNTAX ERROR -A option: Give field abbreviation, slope and intercept\n", GMT_program);
						error = TRUE;
					}
					/* Find what column number this field corresponds to (i.e. depth == 11) */
					col = 0;
					while (strcmp (abbrev, mgd77defs[col].abbrev) && col < MGD77_N_NUMBER_FIELDS)
						col++;
					if (col == MGD77_N_NUMBER_FIELDS) {
						fprintf (stderr, "%s: SYNTAX ERROR -A option: invalid field abbreviation\n", GMT_program);
						error = TRUE;
					}
					if (!error && sscanf (&argv[i][2], "%[^,],%lf,%lf", abbrev, &adjustScale[col], &adjustDC[col]) != 3) {
						fprintf (stderr, "%s: SYNTAX ERROR -A option: Give field abbreviation,slope,intercept\n", GMT_program);
						error = TRUE;
					}
					adjustData = TRUE;
					break;
				case 'B':	/* set nav on land threshold */
					nav_on_land_threshold =  atof (&argv[i][2]);
					break;
				case 'C':	/* set max speed */
					max_speed = atof (&argv[i][2]);
					custom_max_speed = TRUE;
					break;
				case 'D':
					do_regression = FALSE;
					if (argv[i][3] == 'r') report_raw = TRUE;
					if (argv[i][2] == 'd') { /* cruise - grid differences */
						display = "DIFFS";
						n_out_columns = 6;
					}
					else if (argv[i][2] == 'e') { /* E77 error output */
						do_regression = TRUE;
						display = "E77";
						n_out_columns = 6;
					}					
					else if (argv[i][2] == 'E') { /* E77 error output with minimal checking */
						do_regression = FALSE;
						display = "E77";
						n_out_columns = 6;
					}					
					else if (argv[i][2] == 'f') { /* deltaZ and deltaS for each field */
						display = "DFDS";
						n_out_columns = 20;
					}					
					else if (argv[i][2] == 'l') { /* Sniffer limits */
						display = "LIMITS";
						n_out_columns = 4;
					}
					else if (argv[i][2] == 'm') { /* MGD77 output */
						display = "MGD77";
						n_out_columns = 27;
					}
					else if (argv[i][2] == 'n') { /* distance to coast */
						display = "DTC";
						n_out_columns = 4;
					}
					else if (argv[i][2] == 's') { /* gradients */
						display = "SLOPES";
						n_out_columns = 11;
					}
					else if (argv[i][2] == 'v') { /* values */
						display = "VALS";
						n_out_columns = 12;
					}
					else {
						fprintf (stderr, "%s: SYNTAX ERROR:  Unrecognized option -%c%c\n", GMT_program,\
						argv[i][1], argv[i][2]);
						error = TRUE;
					}
					/* Silence all warning messages for data dumps */
					for (j = 0; j<MGD77_N_WARN_TYPES; j++) warn[j] = FALSE;
					M.verbose_dest = 2;		/* 1 = stdout, 2 = stderr */
					break;
#ifdef DEBUG
				case 'F':	/* fake mode (specify field and constant z value in -G - no grid reading */
					simulate = TRUE;
					break;
#endif
				case 'g':	/* Get grid filename and geophysical field name to compare with grid */
					this_grid[n_grids].format = 1; 	/* Mercator grid */
					if (sscanf (&argv[i][2], "%[^,],%[^,],%lf,%d,%lf", this_grid[n_grids].abbrev, this_grid[n_grids].fname, &this_grid[n_grids].scale, &this_grid[n_grids].mode, &this_grid[n_grids].max_lat) < 4) {
						fprintf (stderr, "%s: SYNTAX ERROR -g option: Give field abbreviation, grid file, scale, mode [, and optionally max lat]\n", GMT_program);
						error = TRUE;
					}
				case 'G':	/* Get grid filename and geophysical field name to compare with grid */
					if (!error && this_grid[n_grids].format == 0 && sscanf (&argv[i][2], "%[^,],%s", this_grid[n_grids].abbrev, this_grid[n_grids].fname) != 2) {
						fprintf (stderr, "%s: SYNTAX ERROR -G option: Give field abbreviation and grid file\n", GMT_program);
						error = TRUE;
					}
					else {
						/* Find what column number this field corresponds to (i.e. depth == 11) */
						this_grid[n_grids].col = 0;
						if (! strcmp (this_grid[n_grids].abbrev, "nav")) {
							dist_to_coast = TRUE;
							dtc_index = n_grids;
							n_grids++;
							break;
						}
						while (strcmp (this_grid[n_grids].abbrev, mgd77defs[this_grid[n_grids].col].abbrev) && this_grid[n_grids].col < MGD77_N_NUMBER_FIELDS)
							this_grid[n_grids].col++;
						if (this_grid[n_grids].col == MGD77_N_NUMBER_FIELDS) {
							fprintf (stderr, "%s: SYNTAX ERROR -G option: invalid field abbreviation\n", GMT_program);
							error = TRUE;
						}
						if (!strcmp (this_grid[n_grids].abbrev,"depth")) this_grid[n_grids].sign = -1;
						else this_grid[n_grids].sign = 1;
						n_grids++;
					}
					break;
				case 'H':	/* Force to decimate or not during grid comparison */
					forced = TRUE;
					if (argv[i][2] == 'd')
						decimateData = FALSE;
					else if (argv[i][2] == 'f')
						decimateData = TRUE;
					else if (argv[i][2] == '\0' || argv[i][2] == 'b')
						forced = FALSE;
					else {
						fprintf (stderr, "%s: SYNTAX ERROR:  Unrecognized option -%c%c\n", GMT_program, argv[i][1],\
						argv[i][2]);
						error = TRUE;
					}	
					break;
				case 'I':	/* Pass ranges of data records to ignore for output to E77 */
					if (!error && sscanf (&argv[i][2], "%[^,],%" GMT_LL "d,%" GMT_LL "d", BadSection[n_bad_sections].abbrev, &BadSection[n_bad_sections].start, &BadSection[n_bad_sections].stop) != 3) {
						fprintf (stderr, "%s: SYNTAX ERROR -I option: Give field abbreviation,rec1,recN\n", GMT_program);
						error = TRUE;
					}
					/* Find what column number this field corresponds to (i.e. depth == 11) */
					col = 0;
					while (strcmp (BadSection[n_bad_sections].abbrev, mgd77defs[col].abbrev) && col < MGD77_N_NUMBER_FIELDS)
						col++;
					if (col == MGD77_N_NUMBER_FIELDS) {
						fprintf (stderr, "%s: SYNTAX ERROR -I option: invalid field abbreviation\n", GMT_program);
						error = TRUE;
					}
					bad_sections = TRUE;
					BadSection[n_bad_sections].col = col;
					n_bad_sections++;
					if (n_bad_sections == MAX_BAD_SECTIONS) {
						fprintf (stderr, "%s: SYNTAX ERROR -I option: Max number of sections (%d) reached\n", GMT_program, MAX_BAD_SECTIONS);
						error = TRUE;
					}
					break;
				case 'K':	/* Reverse navigation flags */
					flip_flags = TRUE;
					break;
				case 'L':	/* Overwrite default sniffer limits */
					custom_limit_file = &argv[i][2];
					break;
				case 'N':	/* Change to nautical units instead of metric */
					nautical = TRUE;
					speed_units = "knots";
					distance_units = "nm";
					break;
				case 'P':	/* Specify percent limits for all regression tests */
					percent_limit = atof (&argv[i][2]);
					sprintf (fpercent_limit, gmtdefs.d_format, percent_limit);
					break;
				case 'Q':	/* Interpolation parameters */
					interpolant = BCR_BILINEAR;
					for (j = 2; j < 5 && argv[i][j]; j++) {
						switch (argv[i][j]) {
							case 'n':
								interpolant = BCR_NEARNEIGHBOR; break;
							case 'l':
								interpolant = BCR_BILINEAR; break;
							case 'b':
								interpolant = BCR_BSPLINE; break;
							case 'c':
								interpolant = BCR_BICUBIC; break;
							case '/':
							default:
								threshold = atof (&argv[i][j]);
								if (j == 2 && threshold < GMT_SMALL) interpolant = BCR_NEARNEIGHBOR;
								j = 5; break;
						}
					}
					break;
				case 'S':	/* Specify spatial gradients, time gradients, or value differences along-track */
					if (argv[i][2] == 'd') {
						derivative = "DIFF";
						for (j = 0; j<MGD77_N_NUMBER_FIELDS; j++) maxSlope[j] = mgd77snifferdefs[j].maxTimeGrad;
					}
					else if (argv[i][2] == 's') {
						derivative = "SPACE";
						for (j = 0; j<MGD77_N_NUMBER_FIELDS; j++) maxSlope[j] = mgd77snifferdefs[j].maxSpaceGrad;
					}
					else if (argv[i][2] == 't') {
						derivative = "TIME";
						for (j = 0; j<MGD77_N_NUMBER_FIELDS; j++) maxSlope[j] = mgd77snifferdefs[j].maxTimeGrad;
					}
					else {
						fprintf (stderr, "%s: SYNTAX ERROR:  Unrecognized option -%c%c\n", GMT_program, argv[i][1],\
						argv[i][2]);
						error = TRUE;
					}	
					break;
				case 'T':	/* Specify maximum gap between records */
					custom_maxGap = TRUE;
					maxGap = atof (&argv[i][2]);
					if (maxGap < 0) {
						fprintf (stderr, "%s: SYNTAX ERROR -M option: max gap cannot be negative\n", GMT_program);
						error = TRUE;
					}
					break;					
				case 'W':	/* Choose which warning types to go to stdout (default - all) */
					do_regression = FALSE;
					for (j = 0; j<MGD77_N_WARN_TYPES; j++) warn[j] = FALSE;
					while (GMT_strtok (&argv[i][2], ",", &pos, &c)) {
						if (c == 'v')
							warn[VALUE_WARN] = TRUE;
						else if (c == 'g')
							warn[SLOPE_WARN] = TRUE;
						else if (c == 'o')
							warn[GRID_WARN] = TRUE;
						else if (c == 't')
							warn[TIME_WARN] = TRUE;
						else if (c == 's')
							warn[SPEED_WARN] = TRUE;
						else if (c == 'c')
							warn[TYPE_WARN] = TRUE;
						else if (c == 'x') {
							do_regression = TRUE;
							warn[SUMMARY_WARN] = TRUE;
						} else {
							fprintf (stderr, "%s: SYNTAX ERROR:  Unrecognized option -%c%c\n", GMT_program,\
								 argv[i][1], argv[i][2]);
							error = TRUE;
						}
					}
					custom_warn = TRUE;
					break;					
				default:
					fprintf (stderr, "%s: SYNTAX ERROR:  Unrecognized option -%c\n", GMT_program, argv[i][1]);
					error = TRUE;
					break;
			}
		}
		else
			n_cruises++;
	}

	/* DISPLAY ERROR PAGE */
	if (GMT_give_synopsis_and_exit || argc == 1) {	/* Display usage */
		fprintf(stderr,"mgd77sniffer %s - Along-track quality control of MGD77 cruises\n\n", MGD77_VERSION);
		fprintf (stderr, "usage:  mgd77sniffer <cruises> [-Afieldabbrev,scale,offset] [-Cmaxspd] [-Dd|e|E|f|l|m|s|v][r]\n");
		fprintf (stderr, "\t[-gfieldabbrev,imggrid,scale,mode[,latmax]] [-Gfieldabbrev,grid] [-H] [-Ifieldabbrev,rec1,recN] [-K]\n");
		fprintf (stderr, "\t[-Lcustom_limits_file ] [-N] [-Q[b|c|l|n][[/]<threshold>]] [%s] [-Sd|s|t]\n",GMT_Rgeo_OPT);
		fprintf (stderr, "\t[-Tgap] [-Wc|g|o|s|t|v|x] [-Wc|g|o|s|t|v|x] [-V] [%s]\n\n", GMT_bo_OPT);
		if (GMT_give_synopsis_and_exit) exit (EXIT_FAILURE);
		fprintf (stderr, "\tScan MGD77 files for errors using point-by-point sanity checking,\n");
		fprintf (stderr, "\t\talong-track detection of excessive slopes and comparison of cruise\n");
		fprintf (stderr, "\t\tdata with global bathymetry and gravity grids.");
		fprintf (stderr, "\twhere <cruises> is one or more MGD77 legnames, e.g., 08010001 etc.\n");
		fprintf (stderr, "\n\tOPTIONS:\n");
		fprintf (stderr, "\t-A Apply scale factor and DC adjustment to specified data field. Allows adjustment of\n");
		fprintf (stderr, "\t   cruise data prior to along-track analysis. CAUTION: data must be thoroughly examined\n");
		fprintf (stderr, "\t   before applying these global data adjustments. May not be used for multiple cruises.\n");		
		fprintf (stderr, "\t-B Adjust navigation on land threshold (meters inland) [100]\n");
		fprintf (stderr, "\t-C Set maximum ship speed (10 m/s by default, use -N to indicate knots)\n");
		fprintf (stderr, "\t-D Dump cruise data such as sniffer limits, values, gradients and mgd77 records.\n");
		fprintf (stderr, "\t  -Dd print out cruise-grid differences (requires -G option)\n");
		fprintf (stderr, "\t  -De output formatted error summary for each record. See E77 ERROR FORMAT below.\n");	
		fprintf (stderr, "\t  -DE same as -De but no regression checks will be done.\n");	
		fprintf (stderr, "\t  -Df for each field, output value change and distance (or time with -St) since last observation.\n");	
		fprintf (stderr, "\t  -Dl print out mgd77sniffer default limits (requires no additional arguments)\n");
		fprintf (stderr, "\t  -Dm print out MGD77 format\n\t  -Ds print out gradients\n\t  -Dv print out values\n");
		fprintf (stderr, "\t  -Dn print out distance to coast for each record (requires -gnav or -Gnav)\n");		
		fprintf (stderr, "\t   Append r to include all records (default omits records where navigation errors were detected).\n");
#ifdef DEBUG
		fprintf (stderr, "\t-F Test regression analysis. A simulated grid is created from the ship data using slope\n");
		fprintf (stderr, "\t   and intercept passed through the -G option (i.e., -Gfield,m/b no grid name is passed).\n");
		fprintf (stderr, "\t   These factors are then reflected in regression output. Multiple -G calls allowed.\n");
#endif
		fprintf (stderr, "\t-g Compare cruise data to the specified Sandwell/Smith Mercator grid. Requires valid MGD77\n");
		fprintf (stderr, "\t   field abbreviation followed by a comma, the path (if not in current directory)\n");
		fprintf (stderr, "\t   and grid filename, scale (0.1 or 1), and mode (see mgd77manage for details)\n");
		fprintf (stderr, "\t   Optionally, append max latitude in the IMG file [72.0059773539]. Nav on land\n");
		fprintf (stderr, "\t   test can be activated using -g or -G options and requires a distance to nearest\n");
		fprintf (stderr, "\t   coast grid (i.e., -gnav,/data/GRIDS/dist_to_land.grd) with distance reported in cm.\n");
		fprintf (stderr, "\t-G Compare cruise data to the specified GMT geographic grid. Requires valid MGD77 field abbreviation\n");
		fprintf (stderr, "\t   followed by a comma, then the path (if not in current directory) and grid filename.\n");
		fprintf (stderr, "\t   Excessive offsets are flagged according to maxArea threshold (use -L option to\n");
		fprintf (stderr, "\t   adjust maxArea). Useful for comparing faa or depth to global grids though any MGD77\n");
		fprintf (stderr, "\t   field can be compared to any GMT or IMG compatible grid. Multiple grid comparison is\n");
		fprintf (stderr, "\t   supported by  using separate -G or -g calls for each grid.  See GRID FILE INFO below.\n");
		fprintf (stderr, "\t   Nav on land test can be activated using -g or -G options and requires a distance to\n");
		fprintf (stderr, "\t   nearest coast grid (i.e., -Gnav,/data/GRIDS/dist_to_land.grd) with distance reported\n");
		fprintf (stderr, "\t   in cm.\n");
		fprintf (stderr, "\t-H (with -G|g only) disable (or force) decimation during RLS analysis of ship and gridded data.\n");
		fprintf (stderr, "\t   By default mgd77sniffer analyses both the full and decimated data sets then reports RLS statistics\n");
		fprintf (stderr, "\t   for the higher correlation regression.\n");			
		fprintf (stderr, "\t  -Hb analyze both (default), report better of two.\n");
		fprintf (stderr, "\t  -Hd to disable data decimation (equivalent to -H with no argument).\n");	
		fprintf (stderr, "\t  -Hf to force data decimation.\n");
		fprintf (stderr, "\t-I Give one or more times to specify ranges of data record that should be flagged as bad\n");
		fprintf (stderr, "\t   prior to along-track analysis.  The flag information will be echoed out to E77 files.\n");
		fprintf (stderr, "\t   May not be used for multiple cruises.\n");
		fprintf (stderr, "\t-K Reverse navigation quality flags (good to bad and vice versa). May be necessary when a\n");
		fprintf (stderr, "\t   majority of navigation fixes are erroneously flagged bad, which can happen when a cruise's\n");
		fprintf (stderr, "\t   first navigation fix is extremely erroneous. Caution! This will affect sniffer output and\n");
		fprintf (stderr, "\t   should only be attempted after careful manual navigation review.\n");
		fprintf (stderr, "\t-L Override mgd77sniffer default error detection limits. Supply path and filename of\n");
		fprintf (stderr, "\t   the custom limits file. Rows not beginning with a valid MGD77 field abbreviation are\n");
		fprintf (stderr, "\t   ignored. Field abbreviations are listed below in exact form under MGD77 FIELD INFO.\n");
		fprintf (stderr, "\t   Multiple field limits may be modified using one default file, one field per line.\n");
		fprintf (stderr, "\t   Field min, max, maxGradient and maxArea may be changed for each field. maxGradient\n");
		fprintf (stderr, "\t   pertains to the gradient type selected using the -S option. maxArea is used by the\n");
		fprintf (stderr, "\t   -G option as the threshold for flagging excessive offsets. Dump defaults (-Dd) to\n");
		fprintf (stderr, "\t   view syntax or to quickly create an editable custom limits file.\n");
		fprintf (stderr, "\t   Example custom default file contents (see below for field units):\n");
		fprintf (stderr, "\t\tdepth	0	11000	1000	4500\n");
		fprintf (stderr, "\t\tmag	-800	800	-	-\n");
		fprintf (stderr, "\t\tfaa	-250	250	100	2500\n");
		fprintf (stderr, "\t   Use a dash '-' to retain a default limit.\n");
		fprintf (stderr, "\t   Hint: to test your custom limits, try: mgd77sniffer -Dl -L<yourlimitsfile>\n");
		fprintf (stderr, "\t-N Use nautical units.\n");
		fprintf (stderr, "\t-P Flag regression statistics that are outside the specified confidence level.\n");
		fprintf (stderr, "\t   (i.e., -P5 flags coefficients m, b, rms, and r that fall outside 95%%.)\n");
		fprintf (stderr, "\t-Q Quick mode, use bilinear rather than bicubic [Default] interpolation.\n");
		fprintf (stderr, "\t   Alternatively, select interpolation mode by adding b = B-spline, c = bicubic,\n");
		fprintf (stderr, "\t   l = bilinear, or n = nearest-neighbor.\n");
		fprintf (stderr, "\t   Optionally, append <threshold> in the range [0,1]. [Default = 1 requires all\n");
		fprintf (stderr, "\t   4 or 16 nodes to be non-NaN.], <threshold> = 0.5 will interpolate about 1/2 way\n");
		fprintf (stderr, "\t   from a non-NaN to a NaN node, while 0.1 will go about 90%% of the way, etc.\n");
		fprintf (stderr, "\t   -Q0 will return the value of the nearest node instead of interpolating (Same as -Qn).\n");
		fprintf (stderr, "\t-S Specify gradient type for along-track excessive slope  checking.\n");
		fprintf (stderr, "\t  -Sd Calculate change in z values along track (dz)\n");
		fprintf (stderr, "\t  -Ss Calculate spatial gradients (dz/ds) [default]\n");
		fprintf (stderr, "\t  -St Calculate time gradients (dz/dt)\n");
		fprintf (stderr, "\t-T Set maximum acceptable distance gap between records (km) [5].\n");
		fprintf (stderr, "\t   Set to zero to deactivate gap checking.\n");
		fprintf (stderr, "\t-W Print out only certain warning types. Comma delimit any combination of c|g|o|s|t|v|x:\n");
		fprintf (stderr, "\t   where (c) type code warnings, (g)radient out of range, (o)ffsets from grid (requires -G),\n");
		fprintf (stderr, "\t   (s)peed out of range, (t)ime warnings, (v)alue out of range, (x) warning summaries.\n");
		fprintf (stderr, "\t   By default ALL warning messages are printed. Not allowed with -D option.\n");
		fprintf (stderr, "\t-V runs in verbose mode.\n\n");
		fprintf (stderr, "\t-b output binary data for -D option.  Append d for double and s for single precision [double].\n\n");		
		fprintf (stderr, "\tMGD77 FIELD INFO:\n");
		fprintf (stderr, "\tField\t\t\tAbbreviation\t\tUnits\n");
		fprintf (stderr, "\tTwo-way Travel Time\ttwt\t\t\tsec\n");
		fprintf (stderr, "\tCorrected Depth \tdepth\t\t\tm\n");
		fprintf (stderr, "\tMag Total Field1\tmtf1\t\t\tnT\n");
		fprintf (stderr, "\tMag Total Field2\tmtf2\t\t\tnT\n");
		fprintf (stderr, "\tResidual Magnetic\tmag\t\t\tnT\n");
		fprintf (stderr, "\tDiurnal Correction\tdiur\t\t\tnT\n");
		fprintf (stderr, "\tMag Sensor Depth/Alt\tmsd\t\t\tm\n");		
		fprintf (stderr, "\tObserved Gravity\tgobs\t\t\tmGal\n");
		fprintf (stderr, "\tEotvos Correction\teot\t\t\tmGal\n");		
		fprintf (stderr, "\tfree-air Anomaly\tfaa\t\t\tmGal\n\n");
		fprintf (stderr, "\tGRID FILE INFO:\n");
		fprintf (stderr, "\t-g: Img files must be of Sandwell/Smith signed two-byte integer (i2) type with no header.\n");
		fprintf (stderr, "\t-G: Grid files can be any type of GMT grid file (native or netCDF) with header\n");
		fprintf (stderr, "\tA correctly formatted grid file can be generated as follows:\n");
		fprintf (stderr, "\t   e.g., gmtset GRIDFILE_SHORTHAND TRUE\n");
		fprintf (stderr, "\t\tCreate/edit .gmt_io file to include the following rows:\n");
		fprintf (stderr, "\t\t\t# GMT I/O shorthand file\n");
		fprintf (stderr, "\t\t\t# suffix   format_id scale offset       NaN\n");
		fprintf (stderr, "\t\t\tgrd             0       -       -       -\n");
		fprintf (stderr, "\t\t\ti2              2       -       -       32767\n");
		fprintf (stderr, "\t\tgrdraster 1 -R0/359:55/-90/90 -Getopo5_hdr.i2\n\n");
		fprintf (stderr, "E77 ERROR OUTPUT\n");
		fprintf (stderr, "\tError output is divided into (1) a header containing information globally\n");
		fprintf (stderr, "\tapplicable to the cruise and (2) individual error records summarizing all\n");
		fprintf (stderr, "\tall  errors  encountered in each cruise record.\n");
		fprintf (stderr, "\tError Record Format: <time/distance>  <record  number>  <error code string> <description>\n\n");
		fprintf (stderr, "Example:\n# Cruise 08010039 ID 74010908 MGD77 FILE VERSION: 19801230 N_RECS: 3066\n");
		fprintf (stderr, "# Examined: Wed Oct  3 16:30:13 2007 by mtchandl\n");
		fprintf (stderr, "# Arguments:  -De -Gdepth,/data/GRIDS/etopo5_hdr.i2\n");
		fprintf (stderr, "N Errata table verification status\n");
		fprintf (stderr, "# mgd77manage applies corrections if the errata table is verified (toggle 'N' above to 'Y' after review)\n");
		fprintf (stderr, "# For instructions on E77 format and usage, see http://gmt.soest.hawaii.edu/mgd77/errata.php\n");
		fprintf (stderr, "# Verified by:\n");
		fprintf (stderr, "# Comments:\n");
		fprintf (stderr, "# Errata: Header\n");
		fprintf (stderr, "Y-E-08010039-H13-02: Invalid Magnetics Sampling Rate: (99) [  ]\n");
		fprintf (stderr, "Y-W-08010039-H13-10: Survey year (1975) outside magnetic  reference field IGRF 1965 time range (1965-1970)\n");
		fprintf (stderr, "Y-I-08010039-depth-00: RLS m: 1.00053 b: 0 rms: 127.851 r: 0.973422 sig: 1 dec: 0\n");
		fprintf (stderr, "Y-W-08010039-twt-09: More recent bathymetry correction table available\n");
		fprintf (stderr, "Y-W-08010039-mtf1-10: Integer precision\n");
		fprintf (stderr, "Y-W-08010039-mag-10: Integer precision\n");
		fprintf (stderr, "# Errata: Data\n");
		fprintf (stderr, "08010039	1975-05-10T22:16:05.88 74 C-0-0 NAV: excessive speed\n");
		fprintf (stderr, "\n\tError Class Descriptions\n");
		fprintf (stderr, "\tNAV (navigation):\t0 --> fine\n");
		fprintf (stderr, "\t\tA --> time out of range\n");
		fprintf (stderr, "\t\tB --> time decreasing\n");
		fprintf (stderr, "\t\tC --> excessive speed\n");
		fprintf (stderr, "\t\tD --> above sea level\n");
		fprintf (stderr, "\t\tE --> lat undefined\n");
		fprintf (stderr, "\t\tF --> lon undefined\n\n");
		fprintf (stderr, "\tVAL (value):\t0 --> fine\n");
		fprintf (stderr, "\t\tK --> twt invalid\n");
		fprintf (stderr, "\t\tL --> depth invalid\n");
		fprintf (stderr, "\t\tO --> mtf1 invalid\n");
		fprintf (stderr, "\t\tetc.\n\n");
		fprintf (stderr, "\tGRAD (gradient):\t0 --> fine\n");
		fprintf (stderr, "\t\tK --> d[twt] excessive\n");
		fprintf (stderr, "\t\tL --> d[depth] excessive\n");
		fprintf (stderr, "\t\tO --> d[mtf1] excessive\n");
		fprintf (stderr, "\t\tetc.\n\n");
		fprintf (stderr, "\nEXAMPLES:\n\tAlong-track excessive value and gradient checking:\n\t\tmgd77sniffer 08010001\n");
		fprintf (stderr, "\tDump cruise gradients:\n\t\tmgd77sniffer 08010001 -Ds\n");
		fprintf (stderr, "\tTo compare cruise depth with ETOPO5 bathymetry and gravity with Sandwell/Smith 2 min gravity version 11, try\n");
		fprintf (stderr, "\t\tmgd77sniffer 08010001 -Gdepth,/data/GRIDS/etopo5_hdr.i2 -gfaa,/data/GRIDS/grav.11.2.img,0.1,1\n\n");
		exit (EXIT_FAILURE);
	}
	/* ENSURE VALID USE OF OPTIONS */
	if (n_cruises != 0 && !strcmp(display,"LIMITS")) {
		fprintf(stderr, "%s: ERROR: omit cruise ids for -Dl option.\n", GMT_program);
		exit (EXIT_FAILURE);
	}
	else if (GMT_io.binary[GMT_OUT] && !display) {
		fprintf(stderr, "%s: ERROR: -b option requires -D.\n", GMT_program);
		exit (EXIT_FAILURE);
	}
	else if (custom_warn && strcmp(display,"")) {
		fprintf(stderr, "%s: ERROR: Incompatible options -D and -W.\n", GMT_program);
		exit (EXIT_FAILURE);
	}
	else if (!strcmp(display,"DIFFS") && n_grids == 0) {
		fprintf(stderr, "%s: ERROR: -Dd option requires -G|g.\n", GMT_program);
		exit (EXIT_FAILURE);
	}
	if (east < west || south > north) {
		fprintf(stderr, "%s: ERROR: Region set incorrectly\n", GMT_program);
		exit (EXIT_FAILURE);
	}
	if (adjustData && n_cruises > 1) {
		fprintf(stderr, "%s: ERROR: -A adjustments valid for only one cruise.\n", GMT_program);
		exit (EXIT_FAILURE);
	}
	if (!strcmp(display,"DTC") && ! dist_to_coast) {
		fprintf(stderr, "%s: ERROR: -Dn option requires -Gnav or -gnav.\n", GMT_program);
		exit (EXIT_FAILURE);
	}
	if (simulate && n_grids > 0) {
		for (i = 0; i < n_grids; i++) {
			if (sscanf (this_grid[i].fname, "%lf/%lf", &sim_m[i], &sim_b[i]) != 2) {
				fprintf (stderr, "%s: SYNTAX ERROR -G option: Give m/b for simulated grid.\n", GMT_program);
				exit (EXIT_FAILURE);
			}
		}
	}
	
	n_paths = MGD77_Path_Expand (&M, argv, argc, &list);	/* Get list of requested IDs */

	/* NAUTICAL CONVERSION FACTORS */
	if (nautical) {
		distance_factor = 1.0 / MGD77_METERS_PER_NM; /* meters to nm */
		time_factor = 1.0 / (3600); /* seconds to hours */
		/* Adjust max speed for unit change and user specified max speed */
		if (!custom_max_speed) max_speed = MGD77_MAX_SPEED * distance_factor / time_factor;
		if (!custom_min_speed) min_speed = MGD77_MIN_SPEED * distance_factor / time_factor;
	}

	/* READ AND APPLY CUSTOM LIMITS FILE */
	mgd77snifferdefs[MGD77_YEAR].maxValue = (double) systemTime->tm_year + 1900;
	if (custom_limit_file) {
		if ((custom_fp = GMT_fopen (custom_limit_file, "r")) == NULL) {
			fprintf (stderr, "%s: Could not open custom limit file %s\n", GMT_program, custom_limit_file);
			exit (EXIT_FAILURE);
	 	}
		else {
			while (GMT_fgets (custom_limit_line, BUFSIZ, custom_fp)) {
				GMT_chop (custom_limit_line);					/* Rid the world of CR/LF */
				if (sscanf (custom_limit_line,"%s %s %s %s %s", field_abbrev, tmp_min, tmp_max, tmp_maxSlope, tmp_area) == 5) {
					i = 0;
					while (strcmp (mgd77snifferdefs[i].abbrev, field_abbrev) && i <= MGD77_N_NUMBER_FIELDS) i++;
					if (i <= MGD77_N_NUMBER_FIELDS) {
						if (strcmp(tmp_min, "-"))  mgd77snifferdefs[i].minValue = atof (tmp_min);
						if (strcmp(tmp_max, "-"))  mgd77snifferdefs[i].maxValue = atof (tmp_max);
						if (strcmp(tmp_maxSlope, "-")) maxSlope[i] = atof (tmp_maxSlope);
						if (strcmp(tmp_area, "-"))  mgd77snifferdefs[i].maxArea = atof (tmp_area);
					}
				}
				else {
					fprintf (stderr, "%s: Error in custom limits file [%s]\n", GMT_program, custom_limit_line);
					exit (EXIT_FAILURE);
				}
			}
		}
		GMT_fclose (custom_fp);
	}

	GMT_io.in_col_type[GMT_X] = GMT_IS_LON;
	GMT_io.in_col_type[GMT_Y] = GMT_IS_LAT;

	/* Use GMT time formatting */
	GMT_io.out_col_type[MGD77_TIME] = GMT_IS_ABSTIME;

	/* Adjust data dump for number of grids */
	if (!strcmp(display,"DIFFS"))
		n_out_columns = 3+3*n_grids;

	/* PREPARE DUMP OUTPUT FORMATTING */
	if (!strcmp(display,"VALS") || !strcmp(display,"DIFFS")) {
		for (i = MGD77_LATITUDE; i < (n_out_columns + MGD77_LATITUDE); i++) {
			/* Special lat formatting */
			if (i == MGD77_LATITUDE)
				GMT_io.out_col_type[i-MGD77_LATITUDE] = GMT_IS_LAT;

			/* Special lon formatting */
			else if (i == MGD77_LONGITUDE)
				GMT_io.out_col_type[i-MGD77_LATITUDE] = GMT_IS_LON;

			/* Everything else is float */
			else
				GMT_io.out_col_type[i-MGD77_LATITUDE] = GMT_IS_FLOAT;
		}
	}
	else if (display) {
		for (i = 0; i < n_out_columns; i++) {	
			/* All columns are floats */
			GMT_io.out_col_type[i] = GMT_IS_FLOAT;
		}
	}

	/* PRINT OUT DEFAULT GEOPHYSICAL LIMITS */
	if (!strcmp(display,"LIMITS")) {
		sprintf (buffer, "#abbrev\tmin\tmax\tmaxSlope\tmaxArea\n");
		GMT_fputs (buffer, GMT_stdout);
		for (i = 0; i < MGD77_N_NUMBER_FIELDS; i++) {
			if ((1 << i) & (MGD77_GEOPHYSICAL_BITS + MGD77_CORRECTION_BITS)) {
				sprintf (buffer, "%s%s",mgd77defs[i].abbrev,gmtdefs.field_delimiter);
				GMT_fputs (buffer, GMT_stdout);
				GMT_ascii_output_one (GMT_stdout, mgd77snifferdefs[i].minValue, 0);
				GMT_fputs (gmtdefs.field_delimiter, GMT_stdout);
				GMT_ascii_output_one (GMT_stdout, mgd77snifferdefs[i].maxValue, 1);
				GMT_fputs (gmtdefs.field_delimiter, GMT_stdout);
				GMT_ascii_output_one (GMT_stdout, maxSlope[i], 2);
				GMT_fputs (gmtdefs.field_delimiter, GMT_stdout);
				GMT_ascii_output_one (GMT_stdout, mgd77snifferdefs[i].maxArea, 3);
				GMT_fputs ("\n", GMT_stdout);
			}
		}
		exit (EXIT_FAILURE);
	}

	/* PRINT HEADER FOR SNIFFER DATA DUMPS */
	if (display && !GMT_io.binary[GMT_OUT] && GMT_io.io_header[0]) {
		if (!strcmp(display,"SLOPES")) {
			sprintf (buffer, "#speed(%s)%s",speed_units,gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
			sprintf (buffer, "d[twt]%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
			sprintf (buffer, "d[depth]%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
			sprintf (buffer, "d[mtf1]%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
			sprintf (buffer, "d[mtf2]%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
			sprintf (buffer, "d[mag]%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
			sprintf (buffer, "d[diur]%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
			sprintf (buffer, "d[msd]%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
			sprintf (buffer, "d[gobs]%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
			sprintf (buffer, "d[eot]%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
			sprintf (buffer, "d[faa]\n");	GMT_fputs (buffer, GMT_stdout);
		}
		else if (!strcmp(display,"DFDS")) {
			if (!strcmp(derivative,"TIME")) {
				sprintf (buffer, "#d[twt]%sdt%s",gmtdefs.field_delimiter,gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				sprintf (buffer, "d[depth]%sdt%s",gmtdefs.field_delimiter,gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				sprintf (buffer, "d[mtf1]%sdt%s",gmtdefs.field_delimiter,gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				sprintf (buffer, "d[mtf2]%sdt%s",gmtdefs.field_delimiter,gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				sprintf (buffer, "d[mag]%sdt%s",gmtdefs.field_delimiter,gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				sprintf (buffer, "d[diur]%sdt%s",gmtdefs.field_delimiter,gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				sprintf (buffer, "d[msd]%sdt%s",gmtdefs.field_delimiter,gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				sprintf (buffer, "d[gobs]%sdt%s",gmtdefs.field_delimiter,gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				sprintf (buffer, "d[eot]%sdt%s",gmtdefs.field_delimiter,gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				sprintf (buffer, "d[faa]%sdt\n",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
			}
			else {
				sprintf (buffer, "#d[twt]%sds%s",gmtdefs.field_delimiter,gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				sprintf (buffer, "d[depth]%sds%s",gmtdefs.field_delimiter,gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				sprintf (buffer, "d[mtf1]%sds%s",gmtdefs.field_delimiter,gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				sprintf (buffer, "d[mtf2]%sds%s",gmtdefs.field_delimiter,gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				sprintf (buffer, "d[mag]%sds%s",gmtdefs.field_delimiter,gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				sprintf (buffer, "d[diur]%sds%s",gmtdefs.field_delimiter,gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				sprintf (buffer, "d[msd]%sds%s",gmtdefs.field_delimiter,gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				sprintf (buffer, "d[gobs]%sds%s",gmtdefs.field_delimiter,gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				sprintf (buffer, "d[eot]%sds%s",gmtdefs.field_delimiter,gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				sprintf (buffer, "d[faa]%sds\n",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
			}
		}
		else if (!strcmp(display,"VALS")) {
			sprintf (buffer, "#lat%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
			sprintf (buffer, "lon%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
			sprintf (buffer, "twt%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
			sprintf (buffer, "depth%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
			sprintf (buffer, "mtf1%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
			sprintf (buffer, "mtf2%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
			sprintf (buffer, "mag%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
			sprintf (buffer, "diur%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
			sprintf (buffer, "msd%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
			sprintf (buffer, "gobs%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
			sprintf (buffer, "eot%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
			sprintf (buffer, "faa\n");	GMT_fputs (buffer, GMT_stdout);
		}
		else if (n_grids > 0) {
			if (!strcmp(display,"DIFFS")) {
				sprintf (buffer, "#lat%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				sprintf (buffer, "lon%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				sprintf (buffer, "dist%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				for (i = 0; i < n_grids; i++) {
					sprintf (buffer, "crs_%s%s",this_grid[i].abbrev,gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
					sprintf (buffer, "grd_%s%s",this_grid[i].abbrev,gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
					sprintf (buffer, "diff%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				}
				GMT_fputs ("\n", GMT_stdout);
			}
			else if (!strcmp(display,"DTC")) {
				sprintf (buffer, "#lat%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				sprintf (buffer, "lon%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				sprintf (buffer, "dist%s",gmtdefs.field_delimiter);	GMT_fputs (buffer, GMT_stdout);
				sprintf (buffer, "distToCoast\n");	GMT_fputs (buffer, GMT_stdout);
			}
		}
	}

	/* Open grid files */
	for (i = 0; i < n_grids; i++) {
		if (!simulate)
			/* Open and store grid file */
			read_grid (&this_grid[i], &f[i], west, east, south, north, interpolant, threshold);
	}

	if (n_grids) {
		MaxDiff = (double *) GMT_memory (VNULL, (size_t)n_grids, sizeof (double), "mgd77sniffer");
		iMaxDiff = (int *) GMT_memory (VNULL, (size_t)n_grids, sizeof (int), "mgd77sniffer");
	}

	MGD77_Ignore_Format (MGD77_FORMAT_ANY);	/* Reset to all formats OK, then ... */
	MGD77_Ignore_Format (MGD77_FORMAT_CDF);	/* disallow netCDF MGD77+ files */
	MGD77_Ignore_Format (MGD77_FORMAT_TBL);	/* and plain ASCII tables */

	/* PROCESS CRUISES */
	for (argno = 0; argno < n_paths; argno++) {		/* Process each ID */

		if (MGD77_Open_File (list[argno], &M, MGD77_READ_MODE)) continue;

		if (gmtdefs.verbose) fprintf (stderr, "%s: Now processing cruise %s\n", GMT_program, list[argno]);
		
		if (!strcmp(display,"E77")) {
			sprintf (outfile,"%s.e77",M.NGDC_id);
			if ((fpout = fopen (outfile, "w")) == NULL) {
				fprintf (stderr, "%s: Could not open E77 output file %s\n", GMT_program, outfile);
				exit (EXIT_FAILURE);
			}
	 	}

		/* Read MGD77 header */
		if (MGD77_Read_Header_Record (argv[argno], &M, &H))
			fprintf (stderr, "%s: Cruise %s has no header.\n", GMT_program, list[argno]);

		/* Allocate memory for data records */
		n_alloc = GMT_CHUNK;
		D = (struct MGD77_DATA_RECORD *) GMT_memory (VNULL, (size_t)n_alloc, \
		sizeof (struct MGD77_DATA_RECORD), "mgd77sniffer");

		/* READ DATA RECORDS */
		gotTime = FALSE;
		nvalues = n_nan = M.bit_pattern[0] = 0;
		lowPrecision = lowPrecision5 = 0;
		while (!MGD77_Read_Data_Record_m77 (&M, &D[nvalues])) {
			/* Increase memory allocation if necessary */
			if (nvalues == n_alloc - 1) {
				n_alloc <<= 1;
				D = (struct MGD77_DATA_RECORD *) GMT_memory ((void *)D, (size_t)n_alloc, \
				sizeof (struct MGD77_DATA_RECORD), "mgd77sniffer");
			}
			if (GMT_is_dnan(D[nvalues].time)) n_nan++;
			M.bit_pattern[0] |= D[nvalues].bit_pattern;
			D[nvalues].keep_nav = TRUE;
			nvalues++;
		}

		/* Scale and DC adjust if selected */
		if (adjustData) {
			for (i=0;i<MGD77_N_NUMBER_FIELDS;i++) {
				if (!GMT_is_dnan(adjustScale[i]) || !GMT_is_dnan(adjustDC[i])) {
					for (j=0; j<nvalues;j++)
						D[j].number[i] = D[j].number[i] * adjustScale[i] + adjustDC[i];
					sprintf (text, gmtdefs.d_format, adjustScale[i]);
					fprintf (stderr,"%s (%s) Scaled by %s and ",GMT_program, mgd77defs[i].abbrev,text);
					sprintf (text, gmtdefs.d_format, adjustDC[i]);					
					fprintf (stderr,"%s added\n",text);
				}
			}
		}

		/* Set user-specified flagged observations to NaN before analysis */
		if (bad_sections) {
			for (j=0;j<(int)n_bad_sections;j++) {	/* For each bad section */
				for (i=BadSection[j].start-1;i<BadSection[j].stop;i++) {	/* Loop over the flagged records (adjust -1 for C index) */
					D[i].number[BadSection[j].col] = MGD77_NaN;	/* and set them to NaN */
				}
				if (i == nvalues) M.bit_pattern[0] -= (1 << BadSection[j].col); /* Turn off this field if all values have been flagged as bad */
				fprintf (stderr, "%s (%s) Warning: Resetting %d user-flagged records to NaN prior to analysis\n",list[argno],mgd77snifferdefs[BadSection[j].col].abbrev,(int)i);
			}
		}

		/* Output beginning of E77 header */
		if (!strcmp(display,"E77")) {
			fprintf (fpout, "# Cruise %s ID %s MGD77 FILE VERSION: %.4d%2.2d%2.2d N_RECS: %ld\n",list[argno],D[0].word[0],\
			atoi(H.mgd77[MGD77_ORIG]->File_Creation_Year),atoi(H.mgd77[MGD77_ORIG]->File_Creation_Month),atoi(H.mgd77[MGD77_ORIG]->File_Creation_Day),nvalues);
			sprintf(timeStr,"%s",ctime(&clock));
			timeStr[strlen(ctime(&clock))-1] = '\0';
			fprintf (fpout,"# Examined: %s by %s\n",timeStr,M.user);
			fprintf (fpout,"# Arguments: %s\n",arguments);
			fprintf (fpout,"%c Errata table verification status\n",E77_REJECT);
			fprintf (fpout,"# mgd77manage applies corrections if the errata table is verified (toggle 'N' above to 'Y' after review)\n");
			fprintf (fpout,"# For instructions on E77 format and usage, see http://gmt.soest.hawaii.edu/mgd77/errata.php\n");
			fprintf (fpout,"# Verified by:\n");
			fprintf (fpout,"# Comments:\n");
			fprintf (fpout,"# Errata: Header\n");
		}

		/* Check for time stamps */
		if (n_nan < nvalues) gotTime = TRUE;
		if (n_nan > 0 && n_nan < nvalues) { /* Mixed case */
			if (!strcmp(display,"E77"))
				fprintf (fpout, "%c-%c-%s-time-%.02d: %ld of %ld records contain invalid time\n",\
				E77_APPLY,E77_WARN,list[argno],NAV_TIME_OOR,n_nan,nvalues);
			else if (warn[SUMMARY_WARN]) {
				sprintf (buffer, "%s Warning - %ld of %ld records contain invalid time\n",list[argno],n_nan,nvalues);
				GMT_fputs (buffer, GMT_stdout);
			}
		}
		/* Implicit case n_nan == nvalues, time not reported */

		/* Scan MGD77 header */
		if (!strcmp(display,"E77") || warn[SUMMARY_WARN]) {
			MGD77_Verify_Prep_m77 (&M, &H.meta, D, nvalues);	/* First get key meta-data derived from data records */
			MGD77_Verify_Header (&M, &H, fpout);				/* Then verify the header information */
		}

		/* Re-set variables for this cruise */
		landcruise = FALSE;
		nav_error = TRUE;
		overLandCount = overLandStart = n_bad = utc_offset = 0;
		timeErrorStart = noTimeStart = distanceErrorStart = -1;
		noTimeCount = timeErrorCount = distanceErrorCount = bccCode = 0;
		n_nan = n_wrap = 0;
		offsetArea = (double *) GMT_memory (VNULL, (size_t)n_grids, sizeof (double), "mgd77sniffer");
		offsetStart = (GMT_LONG *)GMT_memory (VNULL, (size_t)n_grids, sizeof (GMT_LONG), "mgd77sniffer");
		offsetLength = (double *) GMT_memory (VNULL, (size_t)n_grids, sizeof (double), "mgd77sniffer");
		offsetSign = (GMT_LONG *) GMT_memory (VNULL, (size_t)n_grids, sizeof (GMT_LONG), "mgd77sniffer");
		prevOffsetSign = (GMT_LONG *)GMT_memory (VNULL, (size_t)n_grids, sizeof (GMT_LONG), "mgd77sniffer");
		range = range2 = date = n_days = 0.0;
		wrapsum = 0.0;
		prevFlag = FALSE;
		mtf1 = TRUE;
		for (i = 0; i<n_grids; i++) {
			offsetArea[i] = offsetLength[i] = 0.0;
			offsetStart[i] = 0;
			offsetSign[i] = prevOffsetSign[i] = FALSE;
		}
		for (i = MGD77_LATITUDE; i<MGD77_N_NUMBER_FIELDS; i++) {
			MGD77_sign_bit[i] = 0;
			if (i == MGD77_MSD) continue;
			/* Turn on low precision bits (will be turned off later if ok) */
			if ((M.bit_pattern[0] & (1 << i) & MGD77_FLOAT_BITS) && i != MGD77_MSD) lowPrecision |= (1 << i);
			if (M.bit_pattern[0] & (1 << i) & MGD77_FLOAT_BITS) lowPrecision5 |= (1 << i);
			duplicates[i] = 0;
		}
		/* Adjust along-track gradient type for time */
		if (!strcmp(derivative,"TIME") && !gotTime) {
			/*derivative = "SPACE";*/
			if (warn[TIME_WARN]) {
				sprintf (buffer, "%s Warning: cruise contains no time - time gradients invalid.\n",GMT_program);
				GMT_fputs (buffer, GMT_stdout);
			}
		}

		/* Allocate memory for error array */
		E = (struct MGD77_ERROR *) GMT_memory ((void *)E, (size_t)nvalues,sizeof (struct MGD77_ERROR), "mgd77sniffer");
		memset ((void *)E, 0, (size_t)(nvalues * sizeof (struct MGD77_ERROR)));

		/* RECURSIVELY CHECK FOR BAD NAVIGATION
		   This algorithm assumes quality navigation at start of cruise. It is
		   theoretically possible that the majority of fixes are flagged when
		   the initial navigation fix is bad. In this case, try flipping flags */
		if (gotTime) {
			if (gmtdefs.verbose == 2)
				fprintf (stderr, "%s: Checking for bad navigation\n", GMT_program);
	
			for (curr = 0; curr < nvalues; curr++) {
				if (GMT_is_dnan(D[curr].time)) {
					if (noTimeStart == -1)
						noTimeStart = (int)curr;
					noTimeCount++;
					E[curr].flags[E77_NAV] |= NAV_UNDEF;
					D[curr].keep_nav = FALSE;
				}
				if (!D[curr].keep_nav) continue;
				if (curr > 0) {
					for (j=curr-1; !D[j].keep_nav && j >= 0; j--) continue; /* Find previous good record */
					if (!D[j].keep_nav) continue; /* No valid previous fix */
					/* Check for Time Zone related errors. Possible errors include failing to adjust the reported
					   hour when adjusting the time zone correction. If a vessel heads East over a time zone
					   boundary, the time zone corrector decrements, so if the reported hour does not increment by one
					   hour then UTC time will decrement one full hour. Conversely, heading West over a time zone
					   boundary, the time zone correction increments, so the reported hour needs to decrement in order
					   to preserve UTC time. We assume that MGD77 files lacking time zone correctors are already
					   reported in UTC. Check if dUTC is one hour ahead or behind dLocalTime. */
					if (D[j].number[MGD77_TZ] != D[curr].number[MGD77_TZ]) {
						if ((fabs(D[curr].time-D[j].time) > fabs((D[curr].time-3600.0*D[curr].number[MGD77_TZ])-(D[j].time-3600.0*D[j].number[MGD77_TZ])))) {
							utc_offset += (int)((D[curr].time-D[j].time) - ((D[curr].time-3600.0*D[curr].number[MGD77_TZ])-(D[j].time-3600.0*D[j].number[MGD77_TZ])));
							E[curr].utc_offset = utc_offset;
							E[curr].flags[E77_NAV] |= NAV_TZ_ERROR;
							if (warn[TIME_WARN]) {
								GMT_ascii_format_one (timeStr, D[curr].time, GMT_io.out_col_type[MGD77_TIME]);
								sprintf (placeStr,"%s %s %ld - Time zone adjustment error (Westbound)",list[argno],timeStr,curr+1);
								if (D[curr].time-D[j].time < ((D[curr].time-3600.0*D[curr].number[MGD77_TZ])-(D[j].time-3600.0*D[j].number[MGD77_TZ])))
									sprintf (placeStr,"%s %s %ld - Time zone adjustment error (Eastbound)",list[argno],timeStr,curr+1);
								sprintf (text, gmtdefs.d_format, D[curr].time-D[j].time);
								printf ("%s: d[UTC_time] - d[local_time] = %.1f - %.1f = %.1f sec.\n",placeStr,(D[curr].time-D[j].time),\
								((D[curr].time-3600.0*D[curr].number[MGD77_TZ])-(D[j].time-3600.0*D[j].number[MGD77_TZ])),\
								((D[curr].time-D[j].time) - ((D[curr].time-3600.0*D[curr].number[MGD77_TZ])-(D[j].time-\
								3600.0*D[j].number[MGD77_TZ]))));
							}
						}
					}
					if (utc_offset != 0) {
						E[curr].flags[E77_NAV] |= NAV_TZ_ERROR;
						E[curr].utc_offset = utc_offset;
						/* Without correcting time zone errors in memory, multiple records in the vicinity of the
						   time zone error will be flagged as non-increasing time or excessive speeds. This offset
						   will not be applied in the vast majority of cruises. */
						/*D[curr].time -= utc_offset * 3600;*/
						n_bad++;
					}
				}
			}

			spike_amplitude = NEG; /* For non-increasing time check - must be set to NEG */
			while (nav_error) {
				prev_speed = 0;
				nav_error = FALSE;
				for (curr = 0; curr < nvalues; curr++) {
					if (!D[curr].keep_nav) continue;
					if (curr > 0) {
						for (j=curr-1; !D[j].keep_nav && j >= 0; j--) continue; /* Find previous good record */
						if (!D[j].keep_nav) continue; /* No valid previous fix */

						/* Check for non-increasing time */
						for (k=curr+1; !D[k].keep_nav && k < nvalues; k++) continue; /* Find next good record */
						if (!D[k].keep_nav) continue; /* No valid next fix */
						/* Check for non-increasing time. This algorithm recursively looks for dips in time (\/), then after they
						   are all flagged searches for spikes (/\) in time. Assumes that the first and last time records are okay.
						   Dips must be checked first (set spike_amplitude to NEG before recursion. Searching spikes before dips
						   will likely cause many good records to be flagged as bad. */
						if ((spike_amplitude == NEG && (D[curr].time-E[curr].utc_offset <= D[j].time-E[j].utc_offset && D[curr].time-E[curr].utc_offset <= D[k].time-E[k].utc_offset)) || \
						    (spike_amplitude == POS && (D[curr].time-E[curr].utc_offset >= D[j].time-E[j].utc_offset && D[curr].time-E[curr].utc_offset >= D[k].time-E[k].utc_offset))) {
							E[curr].flags[E77_NAV] |= NAV_TIME_NONINC;
							D[curr].keep_nav=FALSE;
							nav_error = TRUE;
							n_bad++;
							if (warn[TIME_WARN]) {
								GMT_ascii_format_one (timeStr, D[curr].time, GMT_io.out_col_type[MGD77_TIME]);
								sprintf (placeStr,"%s %s %ld",list[argno],timeStr,curr+1);
								sprintf (text, gmtdefs.d_format, D[curr].time-D[j].time);
								sprintf (buffer, "%s - Time not monotonically increasing (%s sec.)\n",placeStr, text);
								GMT_fputs (buffer, GMT_stdout);
							}
							if (timeErrorStart == -1)
								timeErrorStart = (int)curr;
							timeErrorCount++;
						}
					}

				}
				/* Switch to positive amplitude time spikes after first run through */
				if (nav_error == FALSE && spike_amplitude == NEG) {
					nav_error = TRUE;
					spike_amplitude = POS;
				}
			}
			nav_error = TRUE;
			while (nav_error) {
				prev_speed = 0;
				nav_error = FALSE;
				for (curr = 0; curr < nvalues; curr++) {
					if (D[curr].keep_nav == FALSE) continue;
					if (curr > 0) {
						for (j=curr-1; D[j].keep_nav==FALSE && j >= 0; j--) continue; /* Find previous good record */
						if (D[j].keep_nav == FALSE) continue; /* No valid previous fix */
						/* Check for excessive speed */
						speed = (GMT_great_circle_dist(D[j].number[MGD77_LONGITUDE],D[j].number[MGD77_LATITUDE],D[curr].number[MGD77_LONGITUDE],D[curr].number[MGD77_LATITUDE]) \
								*MGD77_DEG_TO_KM*1000*distance_factor)/(((D[curr].time-E[curr].utc_offset)-(D[j].time-E[j].utc_offset))*time_factor);
						if (fabs(speed)>max_speed) {
							nav_error = TRUE;
							if (warn[SPEED_WARN]) {
								GMT_ascii_format_one (timeStr, D[curr].time, GMT_io.out_col_type[MGD77_TIME]);
								sprintf (placeStr,"%s %s %ld",list[argno],timeStr,curr+1);
								sprintf (text, gmtdefs.d_format, speed);
								sprintf (buffer, "%s - Excessive speed %s %s\n",placeStr, text, speed_units);
								GMT_fputs (buffer, GMT_stdout);
							}
							if (fabs(prev_speed) <= max_speed) { /* Bad nav in current record */
								n_bad++;
								E[curr].flags[E77_NAV] |= NAV_HISPD;
								D[curr].keep_nav = FALSE;
							} else { /* Bad nav in previous record */
								E[j].flags[E77_NAV] |= NAV_HISPD;
								D[j].keep_nav = FALSE;
							}
						} else
							prev_speed = speed;
					}
				}
			}
			if (flip_flags) {
				for (curr = 0; curr < nvalues; curr++) D[curr].keep_nav = (D[curr].keep_nav == FALSE);
				if (!strcmp(display,"E77"))
					fprintf (fpout, "%c-%c-%s-nav-%.2d: Warning: navigation quality flags reversed by user\n",E77_APPLY,E77_WARN,\
					list[argno],E77_HDR_NAV);
				if (warn[SPEED_WARN]) {
					sprintf (buffer, "%s - Warning! Navigation flags flipped by user!\n",list[argno]);
					GMT_fputs (buffer, GMT_stdout);
				}
			}
		}
		/* Allocate memory for distance array */
		distance = (double *) GMT_memory (VNULL, (size_t)nvalues, \
		sizeof (double), "mgd77sniffer");
		distance[0] = 0;
		
		/* Setup output array */
		out = (double **)GMT_memory (VNULL, (size_t)nvalues, sizeof (double *), GMT_program);

		/* PROCESS GRID FILES */
		if (n_grids > 0) {

			/* Allocate memory for 2D arrays */
			G = (double **)GMT_memory (VNULL, (size_t)n_grids, sizeof (double *), GMT_program);	/* grid z values */
			diff = (double **)GMT_memory (VNULL, (size_t)n_grids, sizeof (double *), GMT_program);	/* cruise-grid differences */

			for (i = 0; i < n_grids; i++) {

				G[i] = (double *) GMT_memory (VNULL, (size_t)nvalues, sizeof (double), "mgd77sniffer");

				this_grid[i].g_pts = 0;
				/* Skip if cruise lacks data field */
				if (!(M.bit_pattern[0] & (1 << this_grid[i].col)) && strcmp (this_grid[i].abbrev, "nav")) {
					fprintf (stderr, "%s: Warning: %s field not present in MGD77 file\n", GMT_program, this_grid[i].abbrev);
					continue;
				}

				/* Sample grid at each ship location */
				if (simulate) { /* Test case */
					this_grid[i].g_pts = (int)nvalues;
					for (j = 0; j < nvalues; j++)
						/* Simulate a grid using user-set scale and dc shift (RLS coeffs should match m&b) */
						G[i][j] = D[j].number[this_grid[i].col]*sim_m[i]+sim_b[i];
				}
				else
					this_grid[i].g_pts = sample_grid (&this_grid[i], D, G, f[i], i, nvalues);

				/* Over land check - precedes other grid comparisons involving regression */
				if (dist_to_coast && !strcmp (this_grid[i].abbrev, "nav")) {
					for (j = 0; j < nvalues; j++) {
						if (G[i][j]/100 > 0) {
							E[j].flags[E77_NAV] |= NAV_ON_LAND;
							if (G[i][j]/100 > nav_on_land_threshold) {
								n_bad++;
								if (!landcruise)
									overLandStart = (int)curr;
								landcruise = TRUE;
								overLandCount++;
								D[j].keep_nav = FALSE;
							}
						}
					}
					continue;
				}

				/* Count NaNs */
				this_grid[i].n_nan = 0;
				for (j = 0; j < nvalues; j++) {
					if (GMT_is_dnan(D[j].number[this_grid[i].col]) || GMT_is_dnan(G[i][j])) {
						this_grid[i].n_nan++;
					}
				}

				/* Reverse grid sign if depth */
				if (this_grid[i].sign == -1) {
					for (j = 0; j < nvalues; j++) {
						if (simulate) continue;
						G[i][j] *= this_grid[i].sign;
					}
				}

				/* Allocate memory for ship/grid difference array */
				diff[i] = (double *) GMT_memory (VNULL, (size_t)nvalues, sizeof (double), "mgd77sniffer");
				for (j = 0; j < nvalues; j++)
					/* Compute cruise - grid differences */
					diff[i][j] = D[j].number[this_grid[i].col] - G[i][j];				

				/* Initialize variables */
				for (k=0; k<MGD77_N_STATS; k++) { stat[k] = stat2[k] = 0.0; for (j=0; j<GMT_TEXT_LEN; j++) fstat[k][j]='\0'; }
				tcrit = se = 0;
				newScale = FALSE;
				MaxDiff[i] = 0.0;

				if (this_grid[i].g_pts < 2) {
						fprintf (stderr, "%s: Insufficient grid samples for %s comparison\n", GMT_program, this_grid[i].abbrev);
						continue;
				}
			}
		}

		/* Bad navigation warning */
		if (warn[SUMMARY_WARN] || !strcmp(display,"E77")) {
			if (n_bad > 0) {
				if (!strcmp(display,"E77"))
					fprintf (fpout, "%c-%c-%s-nav-%.2d: Flagged %.2f %% of records with bad navigation\n", \
					E77_APPLY,E77_WARN,list[argno],E77_HDR_NAV,n_bad*100.0/nvalues);
				if (warn[SPEED_WARN]) {
					sprintf (buffer, "%s - Flagged %.2f %% of records with bad navigation", list[argno],n_bad*100.0/nvalues);
					GMT_fputs (buffer, GMT_stdout);
					if ((n_bad*1.0)/nvalues > .25) GMT_fputs (", suggest manual navigation review", GMT_stdout);
					if ((n_bad*1.0)/nvalues > .5) GMT_fputs (", may need to flip flags (see -K option)", GMT_stdout);
					GMT_fputs ("\n", GMT_stdout);
				}
			}
		}
		
		/* SHIP VS GRID RLS REGRESSION */
		if (n_grids > 0) {
			for (i = 0; i < n_grids; i++) {
				if (do_regression && strcmp (this_grid[i].abbrev, "nav")) {
					/* Allocate memory for NaN-free arrays */
					ship_val = (double *) GMT_memory (VNULL, (size_t)(nvalues - this_grid[i].n_nan), sizeof (double), "mgd77sniffer");
					grid_val = (double *) GMT_memory (VNULL, (size_t)(nvalues - this_grid[i].n_nan), sizeof (double), "mgd77sniffer");

					/* Store grid/cruise pairs in NaN-free arrays */
					for (j = k = 0; j < nvalues; j++) {
						if (GMT_is_dnan(D[j].number[this_grid[i].col]) || GMT_is_dnan(G[i][j])) continue;
						ship_val[k] = D[j].number[this_grid[i].col];
						grid_val[k] = G[i][j];
						k++;
					}

					/* Do regression */
					if (k > 2) {
						if (gmtdefs.verbose == 2)
							fprintf (stderr, "%s: Comparing %s and %s using RLS regression\n", GMT_program,this_grid[i].abbrev,this_grid[i].fname);
						if (!decimateData && forced) {
							regress_rls (grid_val, ship_val, nvalues-this_grid[i].n_nan, stat, this_grid[i].col);
							decimated = FALSE;
							tcrit = GMT_tcrit (0.975, (double)(nvalues - this_grid[i].n_nan) - 2.0);
							npts=(nvalues - this_grid[i].n_nan);
						}
						else {
							min = mgd77snifferdefs[this_grid[i].col].minValue;
							max = mgd77snifferdefs[this_grid[i].col].maxValue;
							npts = decimate (grid_val, ship_val, nvalues-this_grid[i].n_nan, min, max, mgd77snifferdefs[this_grid[i].col].delta, &decimated_new, &decimated_orig,&extreme,this_grid[i].abbrev);
							if ((1.0*extreme)/k > .05) { /* Many outliers - decimate again */
								fprintf (stderr,"%s (%s) warning: > 5%% of records outside normal data range - using max bounds for regression\n",list[argno],this_grid[i].abbrev);
								npts = decimate (grid_val, ship_val, nvalues-this_grid[i].n_nan, mgd77snifferdefs[this_grid[i].col].binmin, mgd77snifferdefs[this_grid[i].col].binmax,\
								mgd77snifferdefs[this_grid[i].col].delta, &decimated_new, &decimated_orig, &extreme, this_grid[i].abbrev);
							}
							if (decimateData && forced) {
								regress_rls (decimated_new, decimated_orig, npts, stat, this_grid[i].col);
								decimated = TRUE;
								tcrit = GMT_tcrit (0.975, (double)npts - 2.0);
							}
							else {
								if (npts < 3) {
									regress_rls (grid_val, ship_val, (nvalues - this_grid[i].n_nan), stat, this_grid[i].col);
									decimated = FALSE;
									tcrit = GMT_tcrit (0.975, (double)(nvalues - this_grid[i].n_nan) - 2.0);
									if (gmtdefs.verbose)
										fprintf (stderr, "%s: Regression on undecimated data due to insufficient bins\n",GMT_program);
								} else {
									regress_rls (decimated_new, decimated_orig, npts, stat, this_grid[i].col);
									decimated = TRUE;
									regress_rls (grid_val, ship_val, (nvalues - this_grid[i].n_nan), stat2, this_grid[i].col);
									if ((stat[MGD77_RLS_CORR] < stat2[MGD77_RLS_CORR] && stat2[MGD77_RLS_SIG] == 1.0) || \
										(stat[MGD77_RLS_SIG] == 0.0 && stat2[MGD77_RLS_SIG] == 1.0)) {
										if (gmtdefs.verbose == 2)
											fprintf (stderr, "%s: Regression on undecimated data due to better correlation\n",GMT_program);
										for (j=0; j<MGD77_N_STATS; j++) stat[j] = stat2[j];
										npts=(nvalues - this_grid[i].n_nan);
										decimated = FALSE;
									}
									tcrit = GMT_tcrit (0.975, (double)npts - 2.0);
								}
							}
							GMT_free ((void *)decimated_orig);
							GMT_free ((void *)decimated_new);
						}

						/* Make gmtdef formatted array of rls statistics */
						for (j=0; j<MGD77_N_STATS; j++) sprintf (fstat[j],gmtdefs.d_format,stat[j]);

						/* User specified scale factor/DC shift */
						if (adjustData && !strcmp(display,"E77")) {
							if (fabs(adjustScale[this_grid[i].col]-1.0)>0.0) {
								sprintf (text, gmtdefs.d_format, stat[MGD77_RLS_SLOPE]/adjustScale[this_grid[i].col]);
								fprintf (fpout, "%c-%c-%s-%s-%.02d: Regression slope %s different from 1. Recommended: [%f]\n",E77_APPLY,E77_ERROR,list[argno],\
								this_grid[i].abbrev,E77_HDR_SCALE,text,adjustScale[this_grid[i].col]);
							}
							if (fabs(adjustDC[this_grid[i].col])>0.0) {
								sprintf (text, gmtdefs.d_format, stat[MGD77_RLS_ICEPT]-adjustDC[this_grid[i].col]);
								fprintf (fpout, "%c-%c-%s-%s-%.02d: Regression offset %s different from 0. Recommended: [%f]\n",E77_APPLY,E77_ERROR,\
								list[argno],this_grid[i].abbrev,E77_HDR_DCSHIFT,text,adjustDC[this_grid[i].col]);
							}
						}

						if (warn[SUMMARY_WARN]) {
							sprintf (buffer, "%s (%s) RLS m: %s b: %s rms: %s r: %s sig: %d dec: %d\n",
							list[argno],this_grid[i].abbrev,fstat[MGD77_RLS_SLOPE],fstat[MGD77_RLS_ICEPT],fstat[MGD77_RLS_RMS],fstat[MGD77_RLS_CORR],(int)stat[MGD77_RLS_SIG],(int)decimated);
							GMT_fputs (buffer, GMT_stdout);
						}
						else if (!strcmp(display,"E77"))
							fprintf (fpout, "%c-%c-%s-%s-%.02d: RLS m: %s b: %s rms: %s r: %s sig: %d dec: %d\n",E77_APPLY,E77_INFO,list[argno],this_grid[i].abbrev,\
							E77_HDR_RLS,fstat[MGD77_RLS_SLOPE],fstat[MGD77_RLS_ICEPT],fstat[MGD77_RLS_RMS],fstat[MGD77_RLS_CORR],(int)stat[MGD77_RLS_SIG],(int)decimated);

						/* Analyze regression slope if significant */
						if (stat[MGD77_RLS_SIG] == 1.0 && (GMT_is_dnan(adjustScale[this_grid[i].col]) || adjustScale[this_grid[i].col] == 1.0)) {

							/* Get error range for regression slope */
							range = (tcrit * stat[MGD77_RLS_STD]) / sqrt(stat[MGD77_RLS_SXX]);	/* Draper 1.4.8 */

							if ((stat[MGD77_RLS_SLOPE] <= (1.0-range) || stat[MGD77_RLS_SLOPE] >= (1.0+range)) && fabs(stat[MGD77_RLS_SLOPE]-1.0) > FLT_EPSILON) {
								if (warn[SUMMARY_WARN]) {
									sprintf (buffer, "%s (%s) Slope %s is statistically different from 1\n",\
									list[argno],this_grid[i].abbrev,fstat[MGD77_RLS_SLOPE]);
									GMT_fputs (buffer, GMT_stdout);
								}
								/* Check if regression slope matches common scales (0.1, 10, etc.)  */
								for (j = 0; j < 4; j++) { 	
									if (test_slope[j] >= (stat[MGD77_RLS_SLOPE]-range) && test_slope[j] <= (stat[MGD77_RLS_SLOPE]+range)) {
										sprintf (text, gmtdefs.d_format, test_slope[j]);
										if (!strcmp(display,"E77"))
											fprintf (fpout, "%c-%c-%s-%s-%.02d: Regression slope %s statistically identical to %s. Recommended: [%g]\n",\
											E77_REVIEW,E77_ERROR,list[argno],this_grid[i].abbrev,E77_HDR_SCALE,fstat[MGD77_RLS_SLOPE],text,1/test_slope[j]);
										else if (warn[SUMMARY_WARN]) {
											sprintf (buffer, "%s (%s) Slope %s statistically identical to %s. Recommended: [%g]\n",list[argno],this_grid[i].abbrev,\
											fstat[MGD77_RLS_SLOPE],text,1/test_slope[j]);
											GMT_fputs (buffer, GMT_stdout);
										}

		#ifdef FIX
										/* Apply RLS slope to current field if new scale. */
										for (n = 0; n < nvalues; n++) {
											D[n].number[this_grid[i].col] /= test_slope[j];
											diff[i][n] = D[n].number[this_grid[i].col] - G[i][n]; /* Re-compute cruise - grid differences */
										}
										if (gmtdefs.verbose) {
											sprintf (text, gmtdefs.d_format, test_slope[j]);
											fprintf (stderr, "%s (%s) Warning: Scaled by %s for internal along-track analysis\n",\
											list[argno],this_grid[i].abbrev, text);
										}
		#endif
										newScale = TRUE;
									}
									/* If not depth comparison skip fathom check */
									if (j == 1 && this_grid[i].col != MGD77_DEPTH) break;
								}
								if (!newScale) {
									/* Recommend factor of 10 closest to regression slope, for depth add fathoms ratio check */
									recommended_scale = (strcmp (this_grid[i].abbrev, "depth")) ? pow (10.0, rint (log10 (1.0/fabs(stat[MGD77_RLS_SLOPE])))) : \
									((stat[MGD77_RLS_SLOPE] > 1.5 && stat[MGD77_RLS_SLOPE] < 2.1) ? MGD77_METERS_PER_FATHOM : pow (10.0, rint (log10 (1.0/fabs(stat[MGD77_RLS_SLOPE])))));
									sprintf (text, gmtdefs.d_format, recommended_scale);
									if (!strcmp(display,"E77"))
										fprintf (fpout, "%c-%c-%s-%s-%.02d: Regression slope %s different from 1. Recommended: [%s]\n",E77_REJECT,E77_ERROR,\
										list[argno],this_grid[i].abbrev,E77_HDR_SCALE,fstat[MGD77_RLS_SLOPE],text);
									else if (warn[SUMMARY_WARN]) {
										sprintf (buffer, "%s (%s) Slope %s different from 1. Recommended: [%s]\n",list[argno],this_grid[i].abbrev,\
										fstat[MGD77_RLS_SLOPE],text);
										GMT_fputs (buffer, GMT_stdout);
									}
								}
							}
							else if (fabs(stat[MGD77_RLS_STD]) <= FLT_EPSILON && fabs(stat[MGD77_RLS_SLOPE]-1.0) <= FLT_EPSILON &&\
								fabs(stat[MGD77_RLS_ICEPT]) <= FLT_EPSILON) {
								if (!strcmp(display,"E77"))
									fprintf (fpout, "%c-%c-%s-%s-%.02d: Ship and %s grid appear identical (m=1,b=0,s=0)\n",\
									E77_APPLY,E77_WARN,list[argno],this_grid[i].abbrev,E77_HDR_SCALE,this_grid[i].fname);
								else if (warn[SUMMARY_WARN]) {
									sprintf (buffer, "%s (%s) Ship and %s grid appear identical (m=1,b=0,s=0)\n",\
									list[argno],this_grid[i].abbrev,this_grid[i].fname);
									GMT_fputs (buffer, GMT_stdout);
								}
							}
						}

						/* Check regression intercept */
						if (stat[MGD77_RLS_SIG] == 1.0 && (GMT_is_dnan(adjustDC[this_grid[i].col]) || adjustDC[this_grid[i].col] == 0.0)) {
							range = tcrit * stat[MGD77_RLS_STD] * sqrt(stat[MGD77_RLS_SUMX2]/(npts*stat[MGD77_RLS_SXX]));	/* Draper 1.4.11 */
							if (this_grid[i].col != MGD77_DEPTH && (range <= stat[MGD77_RLS_ICEPT] || -1.0*range >= stat[MGD77_RLS_ICEPT])) {
								if (!strcmp(display,"E77")) {
									if (!GMT_is_dnan(adjustDC[this_grid[i].col])) {
										if (adjustDC[this_grid[i].col] == 0)
											fprintf (fpout, "%c-%c-%s-%s-%.02d: Regression offset %s different from 0.\n",E77_APPLY,E77_WARN,\
											list[argno],this_grid[i].abbrev,E77_HDR_DCSHIFT,fstat[MGD77_RLS_ICEPT]);
										else
											fprintf (fpout, "%c-%c-%s-%s-%.02d: Regression offset %s different from 0. Recommended: [%f]\n",E77_APPLY,E77_ERROR,\
											list[argno],this_grid[i].abbrev,E77_HDR_DCSHIFT,fstat[MGD77_RLS_ICEPT],adjustDC[this_grid[i].col]);
									}
									else
										fprintf (fpout, "%c-%c-%s-%s-%.02d: Regression offset %s different from 0.\n",E77_APPLY,E77_WARN,\
										list[argno],this_grid[i].abbrev,E77_HDR_DCSHIFT,fstat[MGD77_RLS_ICEPT]);
								}
								else if (warn[GRID_WARN]) {
									sprintf (buffer, "%s (%s) Offset different than 0 (%s)\n",list[argno],this_grid[i].abbrev,fstat[MGD77_RLS_ICEPT]);
									GMT_fputs (buffer, GMT_stdout);
								}

		#ifdef FIX
								/* Apply DC shift to current field */
								for (n = 0; n < nvalues; n++) {
									D[n].number[this_grid[i].col] -= stat[MGD77_RLS_ICEPT];
									diff[i][n] = D[n].number[this_grid[i].col] - G[i][n]; /* Re-compute cruise - grid differences */
								}
								if (gmtdefs.verbose)
									fprintf (stderr, "%s (%s) Warning: Offset corrected by %s for internal along-track analysis\n",\
									list[argno],this_grid[i].abbrev, fstat[MGD77_RLS_ICEPT]);
		#endif
							}
						}

						/* Check if removing average IGF80-IGF30 offset from faa regression intercept improves fit to < 2 mGal (or to < 1 sigma) */
						/* This check applies to cruises reporting faa without gobs. More thorough tests are performed for cruises having faa and gobs */
						if (M.bit_pattern[0] & (0 << MGD77_GOBS) && this_grid[i].col == MGD77_FAA && fabs(stat[MGD77_RLS_ICEPT]-H.meta.G1980_1930) < 2.0) {
							if (!strcmp(display,"E77"))
								fprintf (fpout, "%c-%c-%s-%s-%.02d: Free-air anomalies may have been computed using IGF 1930. [Adjust to IGF 1980]\n",\
								E77_REVIEW,E77_ERROR,list[argno],this_grid[i].abbrev,E77_HDR_ANOM_FAA_IGF);
							else if (warn[SUMMARY_WARN]) {
								sprintf (buffer, "%s (%s) Free-air anomalies may have been computed using IGF 1930.\n",list[argno],this_grid[i].abbrev);
								GMT_fputs (buffer, GMT_stdout);
							}
						}

						/* Check if rls grid comparison coefficients are outside percent limits */
						if (stat[MGD77_RLS_SIG] == 1.0 && !adjustData) {
							if (!GMT_is_dnan(percent_limit)) {
								if (this_grid[i].col == MGD77_DEPTH) {
									/* Check depth rls slope (two sided test) */
									for (j = 0; depth_v_grid[j].cd < percent_limit/200.0 && j < RLS_N_DEPTH_ROWS-2; j++);
									for (n = RLS_N_DEPTH_ROWS-1; 1-percent_limit/200.0 < depth_v_grid[n].cd && n > 1; n--);
									if (stat[MGD77_RLS_SLOPE] < depth_v_grid[j+1].m || stat[MGD77_RLS_SLOPE] > depth_v_grid[n-1].m) {
										if (!strcmp(display,"E77"))
											fprintf (fpout, "%c-%c-%s-%s-%.02d: Regression slope (%s) outside %s%% limits.\n",E77_APPLY,E77_WARN,\
											list[argno],this_grid[i].abbrev,E77_HDR_SCALE,fstat[MGD77_RLS_SLOPE],fpercent_limit);
										else if (warn[SUMMARY_WARN]) {
											sprintf (buffer, "%s (%s) Regression slope (%s) outside %s%% limits.\n",\
											list[argno],this_grid[i].abbrev,fstat[MGD77_RLS_SLOPE],fpercent_limit);
											GMT_fputs (buffer, GMT_stdout);
										}
									}
									/* Check depth rls rms (right sided test) */
									for (n = RLS_N_DEPTH_ROWS-1; 1-percent_limit/100.0 < depth_v_grid[n].cd && n > 1; n--);
									if (stat[MGD77_RLS_RMS] > depth_v_grid[k-1].rms) {
										if (!strcmp(display,"E77"))
											fprintf (fpout, "%c-%c-%s-%s-%.02d: Regression rms (%s) outside %s%% limits.\n",E77_APPLY,E77_WARN,\
											list[argno],this_grid[i].abbrev,E77_HDR_RMS,fstat[MGD77_RLS_RMS],fpercent_limit);
										else if (warn[SUMMARY_WARN]) {
											sprintf (buffer, "%s (%s) Regression rms (%s) outside %s%% limits.\n",\
											list[argno],this_grid[i].abbrev,fstat[MGD77_RLS_RMS],fpercent_limit);
											GMT_fputs (buffer, GMT_stdout);
										}
									}
									/* Check depth rls correlation (left sided test) */
									for (j = 0; depth_v_grid[j].cd < percent_limit/100.0 && j < RLS_N_DEPTH_ROWS-2; j++);
									if (stat[MGD77_RLS_CORR] < depth_v_grid[j+1].r) {
										if (!strcmp(display,"E77"))
											fprintf (fpout, "%c-%c-%s-%s-%.02d: Regression correlation (%s) outside %s%% limits.\n",E77_APPLY,E77_WARN,\
											list[argno],this_grid[i].abbrev,E77_HDR_CORR,fstat[MGD77_RLS_CORR],fpercent_limit);
										else if (warn[SUMMARY_WARN]) {
											sprintf (buffer, "%s (%s) Regression correlation (%s) outside %s%% limits.\n",\
											list[argno],this_grid[i].abbrev,fstat[MGD77_RLS_CORR],fpercent_limit);
											GMT_fputs (buffer, GMT_stdout);
										}
									}
								}
								else if (this_grid[i].col == MGD77_FAA) {
									/*check faa rls slope (two sided test) */
									for (j = 0; faa_v_grid[j].cd < percent_limit/200.0 && j < RLS_N_FAA_ROWS-2; j++);
									for (n = RLS_N_FAA_ROWS-1; 1-percent_limit/200.0 < faa_v_grid[n].cd && n > 1; n--);
									if (stat[MGD77_RLS_SLOPE] < faa_v_grid[j+1].m || stat[MGD77_RLS_SLOPE] > faa_v_grid[n-1].m) {
										if (!strcmp(display,"E77"))
											fprintf (fpout, "%c-%c-%s-%s-%.02d: Regression slope (%s) outside %s%% limits.\n",E77_APPLY,E77_WARN,list[argno],\
											this_grid[i].abbrev,E77_HDR_SCALE,fstat[MGD77_RLS_SLOPE],fpercent_limit);
										else if (warn[SUMMARY_WARN]) {
											sprintf (buffer, "%s (%s) Regression slope (%s) outside %s%% limits.\n",\
											list[argno],this_grid[i].abbrev,fstat[MGD77_RLS_SLOPE],fpercent_limit);
											GMT_fputs (buffer, GMT_stdout);
										}
									}
									/*check faa rls intercept (two sided test) */
									if (stat[MGD77_RLS_ICEPT] < faa_v_grid[j+1].b || stat[MGD77_RLS_ICEPT] > faa_v_grid[k-1].b) {
										if (!strcmp(display,"E77"))
											fprintf (fpout, "%c-%c-%s-%s-%.02d: Regression offset (%s) outside %s%% limits.\n",E77_APPLY,E77_WARN,list[argno],\
											this_grid[i].abbrev,E77_HDR_DCSHIFT,fstat[MGD77_RLS_ICEPT],fpercent_limit);
										else if (warn[SUMMARY_WARN]) {
											sprintf (buffer, "%s (%s) Regression offset (%s) outside %s%% limits.\n",\
											list[argno],this_grid[i].abbrev,fstat[MGD77_RLS_ICEPT],fpercent_limit);
											GMT_fputs (buffer, GMT_stdout);
										}
									}
									/*check faa rls rms (right sided test) */
									for (n = RLS_N_FAA_ROWS-1; 1-percent_limit/100.0 < faa_v_grid[n].cd && n > 1; n--);
									if (stat[MGD77_RLS_RMS] > faa_v_grid[n-1].rms) {
										if (!strcmp(display,"E77"))
											fprintf (fpout, "%c-%c-%s-%s-%.02d: Regression rms (%s) outside %s%% limits.\n",E77_APPLY,E77_WARN,list[argno],\
											this_grid[i].abbrev,E77_HDR_RMS,fstat[MGD77_RLS_RMS],fpercent_limit);
										else if (warn[SUMMARY_WARN]) {
											sprintf (buffer, "%s (%s) Regression rms (%s) outside %s%% limits.\n",\
											list[argno],this_grid[i].abbrev,fstat[MGD77_RLS_RMS],fpercent_limit);
											GMT_fputs (buffer, GMT_stdout);
										}
									}
									/*check faa rls correlation (left sided test) */
									for (j = 0; faa_v_grid[j].cd < percent_limit/100.0 && j < RLS_N_FAA_ROWS-2; j++);
									if (stat[MGD77_RLS_CORR] < faa_v_grid[j+1].r) {
										if (!strcmp(display,"E77"))
											fprintf (fpout, "%c-%c-%s-%s-%.02d: Regression correlation (%s) outside %s%% limits.\n",E77_APPLY,E77_WARN,list[argno],\
											this_grid[i].abbrev,E77_HDR_CORR,fstat[MGD77_RLS_CORR],fpercent_limit);
										else if (warn[SUMMARY_WARN]) {
											sprintf (buffer, "%s (%s) Regression correlation (%s) outside %s%% limits.\n",\
											list[argno],this_grid[i].abbrev,fstat[MGD77_RLS_CORR],fpercent_limit);
											GMT_fputs (buffer, GMT_stdout);
										}
									}
								}
							}
						}
					}
					else {
						/* Turn off this empty field */
						if (M.bit_pattern[0] & (1 << this_grid[i].col))	M.bit_pattern[0] -= (1 << this_grid[i].col);
						fprintf (stderr, "%s (%s) Warning: Insufficient bins for regression (%ld found)\n",\
						list[argno],this_grid[i].abbrev, k);
					}
					/* Free up regression array memory */
					GMT_free ((void *)ship_val);
					GMT_free ((void *)grid_val);
				}
			}
		}

		/* REGRESSION ON REPORTED VS RECOMPUTED FAA AND MAG ANOMALIES */
		if (do_regression && (M.bit_pattern[0] & (1 << MGD77_GOBS) &&  (M.bit_pattern[0] & (1 << MGD77_FAA)))) {
			/* CHECK FAA REFERENCE MODEL */
			for (m=0; m<(1+((M.bit_pattern[0] & (1 << MGD77_EOT))>0)); m++) { /* If cruise stores eot then run regression twice */
				n_alloc = GMT_CHUNK;
				new_anom = (double *) GMT_memory (VNULL, (size_t)n_alloc, sizeof (double), "mgd77sniffer");
				old_anom = (double *) GMT_memory (VNULL, (size_t)n_alloc, sizeof (double), "mgd77sniffer");
				for (i = n = 0; i < nvalues; i++) {
					if (GMT_is_dnan(D[i].number[MGD77_GOBS]) || GMT_is_dnan(D[i].number[MGD77_FAA])) continue;
					/* Increase memory allocation if necessary */
					if (n == n_alloc - 1) {
						n_alloc <<= 1;
						new_anom = (double *) GMT_memory ((void *)new_anom, (size_t)n_alloc, sizeof (double), "mgd77sniffer");
						old_anom = (double *) GMT_memory ((void *)old_anom, (size_t)n_alloc, sizeof (double), "mgd77sniffer");
					}
					new_anom[n] = D[i].number[MGD77_GOBS] - MGD77_Theoretical_Gravity (D[i].number[MGD77_LONGITUDE], D[i].number[MGD77_LATITUDE], 4);
					if (m == 1 && !GMT_is_dnan(D[i].number[MGD77_EOT])) new_anom[n] += D[i].number[MGD77_EOT];
					old_anom[n] = D[i].number[MGD77_FAA];
					n++;
				}
				if (gmtdefs.verbose == 2) {
					if (m == 0) fprintf (stderr, "%s: Comparing reported with recomputed (gobs - IGF80) faa using RLS regression\n", GMT_program);
					else fprintf (stderr, "%s: Comparing reported with recomputed (gobs - IGF80 + eot) faa using RLS regression\n", GMT_program);
				}
				if (!decimateData && forced) {
					regress_rls (new_anom, old_anom, n, stat, MGD77_FAA);
					decimated = FALSE;
					tcrit = GMT_tcrit (0.975, (double)n - 2.0);
					npts=n;
				}
				else {
					npts = decimate (new_anom, old_anom, n, mgd77snifferdefs[MGD77_FAA].minValue, mgd77snifferdefs[MGD77_FAA].maxValue,\
					mgd77snifferdefs[MGD77_FAA].delta, &decimated_new, &decimated_orig, &extreme, "nfaa");
					if ((1.0*extreme)/n > .05) { /* Many outliers - decimate again */
						fprintf (stderr,"%s (faa) warning: > 5%% of records outside normal data range - using max bounds for regression\n",list[argno]);
						npts = decimate (new_anom, old_anom, n, mgd77snifferdefs[MGD77_FAA].binmin, mgd77snifferdefs[MGD77_FAA].binmax,\
						mgd77snifferdefs[MGD77_FAA].delta, &decimated_new, &decimated_orig, &extreme, "nfaa");
					}
					if (decimateData && forced) {
						regress_rls (decimated_new, decimated_orig, npts, stat, MGD77_FAA);
						decimated = TRUE;
						tcrit = GMT_tcrit (0.975, (double)npts - 2.0);
					}
					else {
						if (npts < 3) {
							regress_rls (new_anom, old_anom, n, stat, MGD77_FAA);
							decimated = FALSE;
							tcrit = GMT_tcrit (0.975, (double)n - 2.0);
							if (gmtdefs.verbose)
								fprintf (stderr, "%s: Regression on undecimated data due to insufficient bins\n",GMT_program);
						} else {
							regress_rls (decimated_new, decimated_orig, npts, stat, MGD77_FAA);
							decimated = TRUE;
							regress_rls (new_anom, old_anom, n, stat2, MGD77_FAA);
							if ((stat[MGD77_RLS_CORR] < stat2[MGD77_RLS_CORR] && stat2[MGD77_RLS_SIG] == 1.0) || \
								(stat[MGD77_RLS_SIG] == 0.0 && stat2[MGD77_RLS_SIG] == 1.0)) {
								if (gmtdefs.verbose == 2)
									fprintf (stderr, "%s: Regression on undecimated data due to better correlation\n",GMT_program);
								for (k=0; k<MGD77_N_STATS; k++) stat[k] = stat2[k];
								npts=n;
								decimated = FALSE;
							}
							tcrit = GMT_tcrit (0.975, (double)npts - 2.0);
						}
					}
				}
				/* Make gmtdef formatted array of rls statistics */
				for (k=0; k<MGD77_N_STATS; k++) sprintf (fstat[k],gmtdefs.d_format,stat[k]);
				range = (tcrit * stat[MGD77_RLS_STD]) / sqrt(stat[MGD77_RLS_SXX]);	/* Draper 1.4.8 */
				range2 = tcrit * stat[MGD77_RLS_STD] * sqrt(stat[MGD77_RLS_SUMX2]/(n*stat[MGD77_RLS_SXX]));	/* Draper 1.4.11 */
				(m == 1) ? sprintf (text,"+eot ") : sprintf (text," ");
				if (stat[MGD77_RLS_SIG] == 1.0) {
					if (((1.0 < (stat[MGD77_RLS_SLOPE]-range) || 1.0 > (stat[MGD77_RLS_SLOPE]+range)) || (0.0 < (stat[MGD77_RLS_ICEPT]-range2) || 0.0 > (stat[MGD77_RLS_ICEPT]+range2)))) {
						if (!strcmp(display,"E77"))
							fprintf (fpout, "%c-%c-%s-faa-%.02d: Anomaly differs from gobs-IGF80%s(m: %s b: %s rms: %s r: %s sig: %d dec: %d). [Recompute]\n",E77_REVIEW,E77_ERROR,list[argno],\
							(int)(E77_HDR_ANOM_FAA+(m*9)),text,fstat[MGD77_RLS_SLOPE],fstat[MGD77_RLS_ICEPT],fstat[MGD77_RLS_RMS],fstat[MGD77_RLS_CORR],(int)stat[MGD77_RLS_SIG],(int)decimated);
						else if (warn[SUMMARY_WARN]) {
							sprintf (buffer, "%s (faa) anomaly differs from gobs-IGF80%s(m: %s b: %s rms: %s r: %s sig: %d dec: %d)\n",list[argno],text,fstat[MGD77_RLS_SLOPE],\
							fstat[MGD77_RLS_ICEPT],fstat[MGD77_RLS_RMS],fstat[MGD77_RLS_CORR],(int)stat[MGD77_RLS_SIG],(int)decimated);
							GMT_fputs (buffer, GMT_stdout);
						}
					} else {
						if (!strcmp(display,"E77"))
							fprintf (fpout, "%c-%c-%s-faa-%.02d: Anomaly equivalent to gobs-IGF80%s(m: %s b: %s rms: %s r: %s sig: %d dec: %d)\n",E77_APPLY,E77_INFO,list[argno],\
							(int)(E77_HDR_ANOM_FAA+(m*9)),text,fstat[MGD77_RLS_SLOPE],fstat[MGD77_RLS_ICEPT],fstat[MGD77_RLS_RMS],fstat[MGD77_RLS_CORR],(int)stat[MGD77_RLS_SIG],(int)decimated);
						else if (warn[SUMMARY_WARN]) {
							sprintf (buffer, "%s (faa) anomaly statistically the same as gobs-IGF80%s(m: %s b: %s rms: %s r: %s sig: %d dec: %d)\n",list[argno],text,\
							fstat[MGD77_RLS_SLOPE],fstat[MGD77_RLS_ICEPT],fstat[MGD77_RLS_RMS],fstat[MGD77_RLS_CORR],(int)stat[MGD77_RLS_SIG],(int)decimated);
							GMT_fputs (buffer, GMT_stdout);
						}
					}
					if (m == 1 && stat[MGD77_RLS_CORR] > lastCorr) {
						if (!strcmp(display,"E77"))
							fprintf (fpout, "%c-%c-%s-faa-%.02d: gobs may not be corrected for Eotvos (correlation for gobs-IGF80+eot > correlation for gobs-IGF80)\n",E77_APPLY,E77_WARN,list[argno],\
							E77_HDR_ANOM_FAA_EOT);		
						else if (warn[SUMMARY_WARN]) {
							sprintf (buffer, "%s (faa) gobs may not be corrected for Eotvos (correlation for gobs-IGF80+eot > correlation for gobs-IGF80)\n",list[argno]);
							GMT_fputs (buffer, GMT_stdout);
						}
					}
				} else {
					if (!strcmp(display,"E77"))
						fprintf (fpout, "%c-%c-%s-faa-%.02d: Insignificant regression: reported versus gobs-IGF80%s(m: %s b: %s rms: %s r: %s sig: %d dec: %d). [Recompute]\n",E77_REJECT,E77_ERROR,list[argno],\
						(int)(E77_HDR_ANOM_FAA+(m*9)),text,fstat[MGD77_RLS_SLOPE],fstat[MGD77_RLS_ICEPT],fstat[MGD77_RLS_RMS],fstat[MGD77_RLS_CORR],(int)stat[MGD77_RLS_SIG],(int)decimated);
					else if (warn[SUMMARY_WARN]) {
						sprintf (buffer, "%s (faa) insignificant regression: reported versus gobs-IGF80%s(m: %s b: %s rms: %s r: %s sig: %d dec: %d)\n",list[argno],text,fstat[MGD77_RLS_SLOPE],\
						fstat[MGD77_RLS_ICEPT],fstat[MGD77_RLS_RMS],fstat[MGD77_RLS_CORR],(int)stat[MGD77_RLS_SIG],(int)decimated);
						GMT_fputs (buffer, GMT_stdout);
					}
				}
				if (m == 0) lastCorr = stat[MGD77_RLS_CORR];
			}

			/* Try to determine which gravity formula was used */
			grav_formula = MGD77_IGF_1980;
			for (k=0; k<MGD77_N_STATS; k++) stat2[k] = stat[k];
			for (m=MGD77_IGF_1980; m>=MGD77_IGF_1930; m--) { /* Skip Heiskanen 1924 */
				if (m==MGD77_IGF_1967) continue; /* Skip IGF 1967 (can't distinguish between it and IGF 1980 */
				for (i = n = 0; i < nvalues; i++) {
					if (GMT_is_dnan(D[i].number[MGD77_GOBS]) || GMT_is_dnan(D[i].number[MGD77_FAA])) continue;
					new_anom[n] = D[i].number[MGD77_GOBS] - MGD77_Theoretical_Gravity ((int)D[i].number[MGD77_LONGITUDE], (int)D[i].number[MGD77_LATITUDE], (int)m);
					if (stat[MGD77_RLS_CORR] > lastCorr && !GMT_is_dnan(D[i].number[MGD77_EOT])) new_anom[n] += D[i].number[MGD77_EOT];
					old_anom[n] = D[i].number[MGD77_FAA];
					n++;
				}
				if (decimated) {
					npts = decimate (new_anom, old_anom, n, mgd77snifferdefs[MGD77_FAA].minValue, mgd77snifferdefs[MGD77_FAA].maxValue,\
					mgd77snifferdefs[MGD77_FAA].delta, &decimated_new, &decimated_orig, &extreme, "nfaa");
					if ((1.0*extreme)/n > .05) { /* Many outliers - decimate again */
						fprintf (stderr,"%s (faa) warning: > 5%% of records outside normal data range - using max bounds for regression\n",list[argno]);
						npts = decimate (new_anom, old_anom, n, mgd77snifferdefs[MGD77_FAA].binmin, mgd77snifferdefs[MGD77_FAA].binmax,\
						mgd77snifferdefs[MGD77_FAA].delta, &decimated_new, &decimated_orig, &extreme, "nfaa");
					}
					regress_rls (decimated_new, decimated_orig, npts, stat, MGD77_FAA);
					GMT_free ((void *)decimated_orig);
					GMT_free ((void *)decimated_new);
				} else
					regress_rls (new_anom, old_anom, n, stat, MGD77_FAA);
				if (stat[MGD77_RLS_CORR] > stat2[MGD77_RLS_CORR]) {
					for (k=0; k<MGD77_N_STATS; k++) stat2[k] = stat[k];					
					grav_formula = (int)m;
				}
				if (gmtdefs.verbose) fprintf (stderr, "%s: Regression statistics for gravity formula (%ld) test (m: %.3f b: %.3f rms: %.3f r: %.6f sig: %d dec: %d)\n",GMT_program,m,stat[MGD77_RLS_SLOPE],\
					stat[MGD77_RLS_ICEPT],stat[MGD77_RLS_RMS],stat[MGD77_RLS_CORR],(int)stat[MGD77_RLS_SIG],(int)decimated);
			}
			for (k=0; k<MGD77_N_STATS; k++) sprintf (fstat[k],gmtdefs.d_format,stat2[k]);
			if (grav_formula == MGD77_IGF_1930) {
				if (!strcmp(display,"E77"))
					fprintf (fpout, "%c-%c-%s-faa-%.02d: Free-air anomalies may have been computed using IGF 1930 (m: %s b: %s rms: %s r: %s sig: %d dec: %d). [Adjust to IGF 1980]\n",E77_REVIEW,
					E77_ERROR,list[argno],E77_HDR_ANOM_FAA_IGF,fstat[MGD77_RLS_SLOPE],fstat[MGD77_RLS_ICEPT],fstat[MGD77_RLS_RMS],fstat[MGD77_RLS_CORR],(int)stat[MGD77_RLS_SIG],(int)decimated);
				else if (warn[SUMMARY_WARN]) {
					sprintf (buffer, "%s (faa) Free-air anomalies may have been computed using IGF 1930 (m: %s b: %s rms: %s r: %s sig: %d dec: %d). (Consider adjusting to 1980).\n",\
					list[argno],fstat[MGD77_RLS_SLOPE],fstat[MGD77_RLS_ICEPT],fstat[MGD77_RLS_RMS],fstat[MGD77_RLS_CORR],(int)stat[MGD77_RLS_SIG],(int)decimated);
					GMT_fputs (buffer, GMT_stdout);
				}
			}
			GMT_free(new_anom);
			GMT_free(old_anom);

			/* Check if rls coefficients are outside percent limits */
			if (!GMT_is_dnan(percent_limit)) {
				/* check faa v newfaa rls slope */
				for (j = 0; faa_v_newfaa[j].cd < percent_limit/200.0 && j < RLS_N_NEWFAA_ROWS-2; j++);
				for (k = RLS_N_NEWFAA_ROWS-1; 1-percent_limit/200.0 < faa_v_newfaa[k].cd && k > 1; k--);
				if (stat[MGD77_RLS_SLOPE] < faa_v_newfaa[j+1].m || stat[MGD77_RLS_SLOPE] > faa_v_newfaa[k-1].m) {
					if (!strcmp(display,"E77"))
						fprintf (fpout, "%c-%c-%s-faa-%.02d: Recomputed anomaly regression slope (%s) outside %s%% limits.\n",E77_APPLY,E77_WARN,list[argno],\
						E77_HDR_ANOM_FAA,fstat[MGD77_RLS_SLOPE],fpercent_limit);
					else if (warn[SUMMARY_WARN]) {
						sprintf (buffer, "%s (faa) Recomputed anomaly regression slope (%s) outside %s%% limits.\n",\
						list[argno],fstat[MGD77_RLS_SLOPE],fpercent_limit);
						GMT_fputs (buffer, GMT_stdout);
					}
				}
				/* check faa rls intercept */
				if (stat[MGD77_RLS_ICEPT] < faa_v_newfaa[j+1].b || stat[MGD77_RLS_ICEPT] > faa_v_newfaa[k-1].b) {
					if (!strcmp(display,"E77"))
						fprintf (fpout, "%c-%c-%s-faa-%.02d: Recomputed anomaly regression intercept (%s) outside %s%% limits.\n",E77_APPLY,E77_WARN,list[argno],\
						E77_HDR_ANOM_FAA,fstat[MGD77_RLS_ICEPT],fpercent_limit);
					else if (warn[SUMMARY_WARN]) {
						sprintf (buffer, "%s (faa) Recomputed anomaly regression intercept (%s) outside %s%% limits.\n",\
						list[argno],fstat[MGD77_RLS_ICEPT],fpercent_limit);
						GMT_fputs (buffer, GMT_stdout);
					}
				}
				/* check faa rls rms  (right sided test)*/
				for (k = RLS_N_NEWFAA_ROWS-1; 1-percent_limit/100.0 < faa_v_newfaa[k].cd && k > 1; k--);
				if (stat[MGD77_RLS_RMS] > faa_v_newfaa[k-1].rms) {
					if (!strcmp(display,"E77"))
						fprintf (fpout, "%c-%c-%s-faa-%.02d: Recomputed anomaly regression rms (%s) outside %s%% limits.\n",E77_APPLY,E77_WARN,list[argno],\
						E77_HDR_ANOM_FAA,fstat[MGD77_RLS_RMS],fpercent_limit);
					else if (warn[SUMMARY_WARN]) {
						sprintf (buffer, "%s (faa) Recomputed anomaly regression rms (%s) outside %s%% limits.\n",\
						list[argno],fstat[MGD77_RLS_RMS],fpercent_limit);
						GMT_fputs (buffer, GMT_stdout);
					}
				}
				/* check faa rls correlation (left sided test) */
				for (j = 0; faa_v_newfaa[j].cd < percent_limit/100.0 && j < RLS_N_NEWFAA_ROWS-2; j++);
				if (stat[MGD77_RLS_CORR] < faa_v_newfaa[j+1].r) {
					if (!strcmp(display,"E77"))
						fprintf (fpout, "%c-%c-%s-faa-%.02d: Recomputed anomaly regression correlation (%s) outside %s%% limits.\n",E77_APPLY,E77_WARN,list[argno],\
						E77_HDR_ANOM_FAA,fstat[MGD77_RLS_CORR],fpercent_limit);
					else if (warn[SUMMARY_WARN]) {
						sprintf (buffer, "%s (faa) Recomputed anomaly regression correlation (%s) outside %s%% limits.\n",\
						list[argno],fstat[MGD77_RLS_CORR],fpercent_limit);
						GMT_fputs (buffer, GMT_stdout);
					}
				}
			}
		}

		/* CHECK MAG REFERENCE MODEL */
		if (do_regression && ((M.bit_pattern[0] & (1 << MGD77_MTF1)) || (M.bit_pattern[0] & (1 << MGD77_MTF2))) &&  M.bit_pattern[0] & (1 << MGD77_MAG)) {
			if (M.bit_pattern[0] & (1 << MGD77_MTF2)) mtf1 = FALSE;
			n_alloc = GMT_CHUNK;
			new_anom = (double *) GMT_memory (VNULL, (size_t)n_alloc, sizeof (double), "mgd77sniffer");
			old_anom = (double *) GMT_memory (VNULL, (size_t)n_alloc, sizeof (double), "mgd77sniffer");	
			for (i = n = 0; i < nvalues; i++) {
				if (GMT_is_dnan(D[i].number[MGD77_MTF2-(int)mtf1]) || GMT_is_dnan(D[i].number[MGD77_MAG])) continue;
				/* Increase memory allocation if necessary */
				if (n == n_alloc - 1) {
					n_alloc <<= 1;
					new_anom = (double *) GMT_memory ((void *)new_anom, (size_t)n_alloc, sizeof (double), "mgd77sniffer");
					old_anom = (double *) GMT_memory ((void *)old_anom, (size_t)n_alloc, sizeof (double), "mgd77sniffer");
				}
				MGD77_gcal_from_dt (&M, D[i].time, &cal);	/* No adjust for TZ; this is GMT UTC time */
				n_days = (GMT_is_gleap (cal.year)) ? 366.0 : 365.0;	/* Number of days in this year */
				/* Get date as decimal year */
				date = cal.year + cal.day_y / n_days + (cal.hour * GMT_HR2SEC_I + cal.min * GMT_MIN2SEC_I + cal.sec) * GMT_SEC2DAY;
				MGD77_igrf10syn (0, date, 1, 0.0, D[i].number[MGD77_LONGITUDE], D[i].number[MGD77_LATITUDE], IGRF);
				if (GMT_is_dnan(new_anom[n] = D[i].number[MGD77_MTF2-(int)mtf1] - IGRF[MGD77_IGRF_F])) continue;			
				if (!GMT_is_dnan(D[i].number[MGD77_DIUR]))
					new_anom[n] += D[i].number[MGD77_DIUR];
				old_anom[n] = D[i].number[MGD77_MAG];
				n++;
			}
			if (n > 0) { /* must have time records for mag recalculation */
				if (gmtdefs.verbose == 2)
					fprintf (stderr, "%s: Comparing reported and recomputed mag using RLS regression\n", GMT_program);
				if (!decimateData && forced) {
					regress_rls (new_anom, old_anom, n, stat, MGD77_MAG);
					decimated = FALSE;
					tcrit = GMT_tcrit (0.975, (double)n - 2.0);
					npts=n;
				}
				else {
					npts = decimate (new_anom, old_anom, n, mgd77snifferdefs[MGD77_MAG].minValue, mgd77snifferdefs[MGD77_MAG].maxValue,\
					mgd77snifferdefs[MGD77_MAG].delta, &decimated_new, &decimated_orig, &extreme, "nmag");
					if ((1.0*extreme)/n > .05) { /* Many outliers - decimate again */
						fprintf (stderr,"%s (mag) warning: > 5%% of records outside normal data range - using max bounds for regression\n",list[argno]);
						npts = decimate (new_anom, old_anom, n, mgd77snifferdefs[MGD77_MAG].binmin, mgd77snifferdefs[MGD77_MAG].binmax,\
						mgd77snifferdefs[MGD77_MAG].delta, &decimated_new, &decimated_orig, &extreme, "nmag");
					}
					if (decimateData && forced) {
						regress_rls (decimated_new, decimated_orig, npts, stat, MGD77_MAG);
						decimated = TRUE;
						tcrit = GMT_tcrit (0.975, (double)npts - 2.0);
					}
					else {
						if (npts < 3) {
							regress_rls (new_anom, old_anom, n, stat, MGD77_MAG);
							decimated = FALSE;
							tcrit = GMT_tcrit (0.975, (double)n - 2.0);
							if (gmtdefs.verbose)
								fprintf (stderr, "%s: Regression on undecimated data due to insufficient bins\n",GMT_program);
						} else {
							regress_rls (decimated_new, decimated_orig, npts, stat, MGD77_MAG);
							decimated = TRUE;
							regress_rls (new_anom, old_anom, n, stat2, MGD77_MAG);
							if ((stat[MGD77_RLS_CORR] < stat2[MGD77_RLS_CORR] && stat2[MGD77_RLS_SIG] == 1.0) || \
								(stat[MGD77_RLS_SIG] == 0.0 && stat2[MGD77_RLS_SIG] == 1.0)) {
								if (gmtdefs.verbose == 2)
									fprintf (stderr, "%s: Regression on undecimated data due to better correlation\n",GMT_program);
								for (k=0; k<MGD77_N_STATS; k++) stat[k] = stat2[k];
								npts=n;
								decimated = FALSE;
							}
							tcrit = GMT_tcrit (0.975, (double)npts - 2.0);
						}
					}
					GMT_free ((void *)decimated_orig);
					GMT_free ((void *)decimated_new);
				}
				/* Make gmtdef formatted array of rls statistics */
				for (k=0; k<MGD77_N_STATS; k++) sprintf (fstat[k],gmtdefs.d_format,stat[k]);
				/* Check for significant scale */
				range = (tcrit * stat[MGD77_RLS_STD]) / sqrt(stat[MGD77_RLS_SXX]);	/* Draper 1.4.8 */
				range2 = tcrit * stat[MGD77_RLS_STD] * sqrt(stat[MGD77_RLS_SUMX2]/(n*stat[MGD77_RLS_SXX]));	/* Draper 1.4.11 */
				if (stat[MGD77_RLS_SIG] == 1.0) {
					if (((1.0 < (stat[MGD77_RLS_SLOPE]-range) || 1.0 > (stat[MGD77_RLS_SLOPE]+range)) || (0.0 < (stat[MGD77_RLS_ICEPT]-range2) || 0.0 > (stat[MGD77_RLS_ICEPT]+range2)))) {
						if (!strcmp(display,"E77"))
							fprintf (fpout, "%c-%c-%s-mag-%.2d: Anomaly differs from mtf%d-IGRF (m: %s b: %s rms: %s r: %s sig: %d dec: %d). [Recompute]\n",E77_REVIEW,E77_ERROR,list[argno],\
							E77_HDR_ANOM_MAG,2-(int)mtf1,fstat[MGD77_RLS_SLOPE],fstat[MGD77_RLS_ICEPT],fstat[MGD77_RLS_RMS],fstat[MGD77_RLS_CORR],(int)stat[MGD77_RLS_SIG],(int)decimated);
						else if (warn[SUMMARY_WARN]) {
							sprintf (buffer, "%s (mag) anomaly differs from mtf%d-IGRF (m: %s b: %s rms: %s r: %s sig: %d dec: %d)\n",list[argno],2-(int)mtf1,\
							fstat[MGD77_RLS_SLOPE],fstat[MGD77_RLS_ICEPT],fstat[MGD77_RLS_RMS],fstat[MGD77_RLS_CORR],(int)stat[MGD77_RLS_SIG],(int)decimated);
							GMT_fputs (buffer, GMT_stdout);
						}
					} else {
						if (warn[SUMMARY_WARN]) {
							sprintf (buffer, "%s (mag) anomaly same as expected (m: %s b: %s rms: %s r: %s sig: %d dec: %d)\n",list[argno],fstat[MGD77_RLS_SLOPE],\
							fstat[MGD77_RLS_ICEPT],fstat[MGD77_RLS_RMS],fstat[MGD77_RLS_CORR],(int)stat[MGD77_RLS_SIG],(int)decimated);
							GMT_fputs (buffer, GMT_stdout);
						}
						else if (!strcmp(display,"E77"))
							fprintf (fpout, "%c-%c-%s-mag-%.02d: Anomaly equivalent to mtf%d-IGRF (m: %s b: %s rms: %s r: %s sig: %d dec: %d)\n",E77_APPLY,E77_INFO,list[argno],\
							E77_HDR_ANOM_MAG,2-(int)mtf1,fstat[MGD77_RLS_SLOPE],fstat[MGD77_RLS_ICEPT],fstat[MGD77_RLS_RMS],fstat[MGD77_RLS_CORR],(int)stat[MGD77_RLS_SIG],(int)decimated);
					}
				} else {
					if (warn[SUMMARY_WARN]) {
						sprintf (buffer, "%s (mag) recalculated anomaly regression insignificant (m: %s b: %s rms: %s r: %s sig: %d dec: %d)\n",list[argno],fstat[MGD77_RLS_SLOPE],\
						fstat[MGD77_RLS_ICEPT],fstat[MGD77_RLS_RMS],fstat[MGD77_RLS_CORR],(int)stat[MGD77_RLS_SIG],(int)decimated);
						GMT_fputs (buffer, GMT_stdout);
					}
				}
			} else {
				sprintf (buffer, "%s (mag) unable to recompute anomalies\n",list[argno]);
				GMT_fputs (buffer, GMT_stdout);
			}
			GMT_free (new_anom);
			GMT_free(old_anom);

			/* Check if rls coefficients are outside percent limits */
			if (!GMT_is_dnan(percent_limit)) {
				/* check mag v newmag rls slope */
				for (j = 0; mag_v_newmag[j].cd < percent_limit/200.0 && j < RLS_N_NEWMAG_ROWS-2; j++);
				for (k = RLS_N_NEWMAG_ROWS-1; 1-percent_limit/200.0 < mag_v_newmag[k].cd && k > 1; k--);
				if (stat[MGD77_RLS_SLOPE] < mag_v_newmag[j+1].m || stat[MGD77_RLS_SLOPE] > mag_v_newmag[k-1].m) {
					if (!strcmp(display,"E77"))
						fprintf (fpout, "%c-%c-%s-mag-%.02d: Recomputed anomaly regression slope (%s) outside %s%% limits.\n",E77_APPLY,E77_WARN,\
						list[argno],E77_HDR_ANOM_MAG,fstat[MGD77_RLS_SLOPE],fpercent_limit);
					else if (warn[SUMMARY_WARN]) {
						sprintf (buffer, "%s (mag) Recomputed anomaly regression slope (%s) outside %s%% limits.\n",\
						list[argno],fstat[MGD77_RLS_SLOPE],fpercent_limit);
						GMT_fputs (buffer, GMT_stdout);
					}
				}
				/* check mag rls intercept */
				if (stat[MGD77_RLS_ICEPT] < mag_v_newmag[j+1].b || stat[MGD77_RLS_ICEPT] > mag_v_newmag[k-1].b) {
					if (!strcmp(display,"E77"))
						fprintf (fpout, "%c-%c-%s-mag-%.02d: Recomputed anomaly regression intercept (%s) outside %s%% limits.\n",E77_APPLY,E77_WARN,\
						list[argno],E77_HDR_ANOM_MAG,fstat[MGD77_RLS_ICEPT],fpercent_limit);
					else if (warn[SUMMARY_WARN]) {
						sprintf (buffer, "%s (mag) Recomputed anomaly regression intercept (%s) outside %s%% limits.\n",\
						list[argno],fstat[MGD77_RLS_ICEPT],fpercent_limit);
						GMT_fputs (buffer, GMT_stdout);
					}
				}
				/* check mag rls rms (right sided test) */
				for (k = RLS_N_NEWMAG_ROWS-1; 1-percent_limit/100.0 < mag_v_newmag[k].cd && k > 1; k--);
				if (stat[MGD77_RLS_RMS] > mag_v_newmag[k-1].rms) {
					if (!strcmp(display,"E77"))
						fprintf (fpout, "%c-%c-%s-mag-%.02d: Recomputed anomaly regression rms (%s) outside %s%% limits.\n",E77_APPLY,E77_WARN,\
						list[argno],E77_HDR_ANOM_MAG,fstat[MGD77_RLS_RMS],fpercent_limit);
					else if (warn[SUMMARY_WARN]) {
						sprintf (buffer, "%s (mag) Recomputed anomaly regression rms (%s) outside %s%% limits.\n",\
						list[argno],fstat[MGD77_RLS_RMS],fpercent_limit);
						GMT_fputs (buffer, GMT_stdout);
					}
				}
				/* check mag rls correlation (left sided test) */
				for (j = 0; mag_v_newmag[j].cd <percent_limit/100.0 && j < RLS_N_NEWMAG_ROWS-2; j++);
				if (stat[MGD77_RLS_CORR] < mag_v_newmag[j+1].r) {
					if (!strcmp(display,"E77"))
						fprintf (fpout, "%c-%c-%s-mag-%.02d: Recomputed anomaly regression correlation (%s) outside %s%% limits.\n",E77_APPLY,E77_WARN,\
						list[argno],E77_HDR_ANOM_MAG,fstat[MGD77_RLS_CORR],fpercent_limit);
					else if (warn[SUMMARY_WARN]) {
						sprintf (buffer, "%s (mag) Recomputed anomaly regression correlation (%s) outside %s%% limits.\n",\
						list[argno],fstat[MGD77_RLS_CORR],fpercent_limit);
						GMT_fputs (buffer, GMT_stdout);
					}
				}
			}
		}		

		/* CHECK SANITY ALONG-TRACK */
		if (gmtdefs.verbose == 2)
			fprintf (stderr, "%s: Checking for along-track errors\n", GMT_program);
		lastLat = D[0].number[MGD77_LATITUDE];
		lastLon = D[0].number[MGD77_LONGITUDE];
		ds = distance[0] = 0.0;
		for (curr = 0; curr < nvalues; curr++) {

			/* Compute distance (keep units in km) */
			/* Use GMT great circle distance function to calculate arc length between */
			/* current and previous record (in degrees) then convert to km and accumulate */
			thisLat = D[curr].number[MGD77_LATITUDE];
			thisLon = D[curr].number[MGD77_LONGITUDE];
			if (curr > 0) {
				lastLat = D[curr-1].number[MGD77_LATITUDE];
				lastLon = D[curr-1].number[MGD77_LONGITUDE];
				ds = GMT_great_circle_dist (lastLon, lastLat, thisLon, thisLat) * MGD77_DEG_TO_KM;
				distance[curr] = ds + distance[curr-1];
			}

			/* Initialize output array for this record */
			out[curr] = (double *)GMT_memory (VNULL, (size_t)n_out_columns, sizeof (double), GMT_program);
			for (i = 0; i < n_out_columns; i++) out[curr][i] = MGD77_NaN;

			/* Store lat, lon, along-track distance, and distance from coast in output array */
			if (!strcmp(display,"DTC") && dist_to_coast && (D[curr].keep_nav || report_raw)) {
				out[curr][0] = D[curr].number[MGD77_LATITUDE];
				out[curr][1] = D[curr].number[MGD77_LONGITUDE];
				out[curr][2] = distance[curr]*distance_factor;
				out[curr][3] = G[dtc_index][curr]/100; /* report dtc in meters */
			}

			/* Create the current time string formatted according to gmtdefaults */
			if (gotTime)
				GMT_ascii_format_one (timeStr, D[curr].time, GMT_io.out_col_type[MGD77_TIME]);
			else
				GMT_ascii_format_one (timeStr, distance[curr], GMT_io.out_col_type[GMT_IS_FLOAT]);
				
			/* Create the location portion of the verbose data warning string (not for E77) */
			sprintf (placeStr,"%s %s %ld",list[argno],timeStr,curr+1);

			/* Check for time out of range */
			if (D[curr].time > maxTime || D[curr].time < \
			MGD77_rdc2dt (&M, GMT_rd_from_gymd(irint(mgd77snifferdefs[MGD77_YEAR].minValue), 1, 1),0.0)) {
				E[curr].flags[E77_NAV] |= NAV_TIME_OOR;
				if (warn[TIME_WARN]) {
					sprintf (buffer, "%s - Time out of range\n",placeStr);
					GMT_fputs (buffer, GMT_stdout);
				}
			}

			nwords = nout = 0;

			/* Store latitude and longitude in the output array */
			if (!strcmp(display,"VALS") && (D[curr].keep_nav || report_raw)) {
				out[curr][nout] = D[curr].number[MGD77_LATITUDE];
				nout++;
				out[curr][nout] = D[curr].number[MGD77_LONGITUDE];
				nout++;
			}

			/* SCAN FOR VALUES OUT OF RANGE (POINT-BY-POINT Error Checking) */
			/* Check the 24 numeric fields (start with latitude)*/
   			for (i = MGD77_RECTYPE; i < MGD77_N_NUMBER_FIELDS; i++) {

				/* Store cruise values in the output array */
				if ((MGD77_this_bit[i] & (MGD77_GEOPHYSICAL_BITS | MGD77_CORRECTION_BITS)) && !strcmp(display,"VALS") && (D[curr].keep_nav || report_raw)) {
					out[curr][nout] = D[curr].number[i];
					nout++;
				}

				/* Only scan fields present in this cruise */
				if (M.bit_pattern[0] & (1 << i)) {
					switch (i) {
						case (MGD77_RECTYPE):
							if ((int) D[curr].number[i] != 3 && (int) D[curr].number[i] != 5) {
								E[curr].flags[E77_VALUE] |= (1 << i);
								if (warn[TYPE_WARN]) {
									sprintf (buffer, "%s - Invalid code %s [%d]\n",\
									placeStr,mgd77defs[i].abbrev, (int) D[curr].number[i]);
									GMT_fputs (buffer, GMT_stdout);
								}
							}
							break;
						case (MGD77_PTC):
						case (MGD77_BTC):
							if (((int) D[curr].number[i] != 1 && (int) D[curr].number[i] != 3 &&\
							(int) D[curr].number[i] != 9)) {
								E[curr].flags[E77_VALUE] |= (1 << i);
								if (warn[TYPE_WARN]) {
									sprintf (buffer, "%s - Invalid code %s [%d]\n",\
									placeStr,mgd77defs[i].abbrev, (int) D[curr].number[i]);
									GMT_fputs (buffer, GMT_stdout);
								}
							}
							break;
						case (MGD77_MSENS):
							if ((int)D[curr].number[i] != 1 && (int)D[curr].number[i] != 2 &&\
							(int) D[curr].number[i] != 9) {
								E[curr].flags[E77_VALUE] |= (1 << i);
								if (warn[TYPE_WARN]) {
									sprintf (buffer, "%s - Invalid code %s [%d]\n", \
									placeStr,mgd77defs[i].abbrev, (int) D[curr].number[i]);
									GMT_fputs (buffer, GMT_stdout);
								}
							}
							break;
						case (MGD77_NQC):
							if ((int) D[curr].number[i] != 5 && (int) D[curr].number[i] != 6 &&\
							(int) D[curr].number[i] != 9) {
								E[curr].flags[E77_VALUE] |= (1 << i);
								if (warn[TYPE_WARN]) {
									sprintf (buffer, "%s - Invalid code %s [%d]\n",placeStr,\
									mgd77defs[i].abbrev, (int) D[curr].number[i]);
									GMT_fputs (buffer, GMT_stdout);
								}
							}
							break;
						case (MGD77_BCC):
							bccCode = irint (D[curr].number[i]);
							if (M.bit_pattern[0] & MGD77_TWT_BIT || M.bit_pattern[0] & MGD77_DEPTH_BIT) {
								if (bccCode < 1 || bccCode > 55) {
									switch ((int) bccCode) {
										case 59:	/* Matthews', no zone */
										case 60:	/* S. Kuwahara Formula */
										case 61:	/* Wilson Formula */
										case 62:	/* Del Grosso Formula */
										case 63:	/* Carter's Tables */
										case 88:	/* Other */
										case 98:	/* Unknown */
										case 99:	/* Unspecified */
											break;
										default:
											E[curr].flags[E77_VALUE] |= (1 << i);
											if (warn[TYPE_WARN]) {
												sprintf (buffer, "%s - Invalid code %s [%d]\n",placeStr,\
												mgd77defs[i].abbrev, (int) bccCode);
												GMT_fputs (buffer, GMT_stdout);
											}
											break;
									}
								}
							}
							break;
						case (MGD77_DAY):	/* Separate case since # of days in a month varies */
							if (!GMT_is_dnan (D[curr].number[i])) {
								if ((E[curr].flags[E77_VALUE] & (1 << MGD77_YEAR)) || (E[curr].flags[E77_VALUE] & (1 << MGD77_MONTH)))
									last_day = irint (mgd77snifferdefs[i].maxValue);	/* Year or month has error so we use 31 as last day in this month */
								else
									last_day = (int)GMT_gmonth_length (irint(D[curr].number[MGD77_YEAR]), irint(D[curr].number[MGD77_MONTH]));			/* Number of day in the specified month */

								if (GMT_is_dnan (D[curr].number[i]) && (D[curr].number[i] < mgd77snifferdefs[i].minValue || D[curr].number[i] > last_day)) {
									E[curr].flags[E77_VALUE] |= (1 << i);
									if (warn[VALUE_WARN]) {
										sprintf (text, gmtdefs.d_format, D[curr].number[i]);
										sprintf (buffer, "%s - %s out of range [%s]\n", placeStr, mgd77defs[i].abbrev, text);
										GMT_fputs (buffer, GMT_stdout);
									}
								}
							}
							break;
						case (MGD77_LONGITUDE): /* Handle longitudes in 180-360 range */
							if (!GMT_is_dnan (D[curr].number[i]) && D[curr].number[i] > mgd77snifferdefs[i].maxValue && D[curr].number[i] <= 360.0) {
								if (warn[VALUE_WARN]) {
									sprintf (text, gmtdefs.d_format, D[curr].number[i]);								
									sprintf (buffer, "%s - %s adjusted %s to +/- 180\n", placeStr, mgd77defs[i].abbrev, text);
									GMT_fputs (buffer, GMT_stdout);
								}
								D[curr].number[i] -= 360.0;
							}
						default:
							/* Verify that measurements are within range */
							if (!GMT_is_dnan (D[curr].number[i]) && (D[curr].number[i] < mgd77snifferdefs[i].minValue ||\
							D[curr].number[i] > mgd77snifferdefs[i].maxValue)) {
								E[curr].flags[E77_VALUE] |= (1 << i);
								if (warn[VALUE_WARN]) {
									sprintf (text, gmtdefs.d_format, D[curr].number[i]);
									sprintf (buffer, "%s - %s out of range [%s]\n", placeStr, mgd77defs[i].abbrev, text);
									GMT_fputs (buffer, GMT_stdout);
								}
							}
							if ((i == MGD77_LATITUDE || i == MGD77_LONGITUDE) && GMT_is_dnan(D[curr].number[i])) {
								E[curr].flags[E77_NAV] |= NAV_UNDEF;
								if (warn[VALUE_WARN]) {
									sprintf (buffer, "%s - %s cannot be nine-filled\n", placeStr, mgd77defs[i].abbrev);
									GMT_fputs (buffer, GMT_stdout);
								}
							}
							/* Record sign of current value */
							if ((1 << i) & (MGD77_GEOPHYSICAL_BITS + MGD77_CORRECTION_BITS) && !GMT_is_dnan(D[curr].number[i])) {
								if (D[curr].number[i] < 0)
									MGD77_sign_bit[i] |= (MGD77_NEG_BIT);
								else if (D[curr].number[i] > 0)
									MGD77_sign_bit[i] |= (MGD77_POS_BIT);
								else
									MGD77_sign_bit[i] |= (MGD77_ZERO_BIT);
							}
							break;
					}
				}
			}

			/* Along-Track Excessive Slope Error Checking */
			if (curr > 0) {

				/* Check dt */
				if (!GMT_is_dnan(D[curr].time) && !GMT_is_dnan(D[curr-1].time))
					dt = D[curr].time-E[curr].utc_offset - D[curr-1].time-E[curr-1].utc_offset;
				else	/* Time not specified */
					dt = MGD77_NaN;
				if (!GMT_is_dnan(dt) && dt <= 0) { /* Non-increasing time */
					if (dt == 0) {
						if (warn[TIME_WARN]) {
							sprintf (text, gmtdefs.d_format, dt);
							sprintf (buffer, "%s - Time not monotonically increasing (%s sec.)\n",placeStr, text);
							GMT_fputs (buffer, GMT_stdout);
						}
						dt = MGD77_NaN;
					} else {
						if (warn[TIME_WARN]) {
							sprintf (text, gmtdefs.d_format, dt);
							sprintf (buffer, "%s - Time decreasing (%s sec.)\n",placeStr, text);
							GMT_fputs (buffer, GMT_stdout);
						}
					}
				}

				/* Calculate speed */
				if (dt != 0)
					speed = (ds*1000*distance_factor)/(dt*time_factor);
				else
					speed = MGD77_NaN;

#ifdef HISTOGRAM_MODE
				/* Use this to print excessive slopes to stderr for
				tracking winning cruises when running histogram scripts */
				if (!GMT_is_dnan(speed) && fabs(speed) > max_speed) {
					E[curr].flags[E77_NAV] |= NAV_HISPD;
					fprintf (stderr, "%s - Excessive speed %f %s\n",placeStr, speed, speed_units);
				}
#endif

				/* Store speed in the output array */
				nout = 0;
				if (!strcmp(display,"SLOPES") && (D[curr].keep_nav || report_raw)) {
					if (!GMT_is_dnan(speed))
						out[curr][nout] = speed;
					nout++;
				}

				/* Check slope values for non-time geophysical measurements */
				for (i = MGD77_LATITUDE; i < MGD77_N_NUMBER_FIELDS; i++) {

					/* Only scan floating point fields present in this cruise */						
					if (M.bit_pattern[0] & (1 << i) & MGD77_FLOAT_BITS) {

						/* Compute the difference between current and previous value */
						if (GMT_is_dnan(D[curr].number[i])) {
							gradient = MGD77_NaN;
							dvalue = MGD77_NaN;
						}
						else {
							/* Search backward to find a non-empty record for the same field. */
							/* Hope it doesn't have to search too far */
							for (j = 1; GMT_is_dnan(D[curr-j].number[i]) && curr-j > 0; j++)
								if (j > MGD77_MAX_SEARCH) break;
							dvalue = D[curr].number[i]-D[curr-j].number[i];
							lastLat = D[curr-j].number[MGD77_LATITUDE];
							lastLon = D[curr-j].number[MGD77_LONGITUDE];
							/* Calculate ds & dt between current and last valid record */
							/* Note: ds may be different for each field */
							ds = GMT_great_circle_dist (lastLon, lastLat, thisLon, thisLat) * MGD77_DEG_TO_KM;
							dt = D[curr].time-E[curr].utc_offset - D[curr-j].time-E[curr-j].utc_offset;

							/* Set to nan if a gap is detected (unless gap skipping is turned off) */
							if (ds > maxGap && maxGap != 0)
								dvalue = MGD77_NaN;

							/* Check for PDR wrap around */
							if (i == MGD77_TWT && (M.bit_pattern[0] & (1 << i)) && !GMT_is_dnan(dvalue) && fabs(dvalue) > 4.0) {
								wrapsum += fabs(dvalue);
								n_wrap++;
							}

							/* Compute gradient */
							if (!strcmp(derivative,"SPACE")) {
								if (ds >= MGD77_NAV_PRECISION_KM && \
								   ((!GMT_is_dnan(speed) && speed >= min_speed) || (GMT_is_dnan(speed) && ds >= MGD77_MIN_DS)))
									gradient = dvalue / ds;
								else
									gradient = MGD77_NaN;
							}
							else if (!strcmp(derivative,"DIFF"))
								gradient = dvalue;
							else { /* Gradient with respect to time */
								if (dt > 0)
									gradient = dvalue / dt;
								else
									gradient = MGD77_NaN;
							}

							/* First Derivative Sanity Check */
							if (fabs(gradient) > maxSlope[i]) {
								E[curr].flags[E77_SLOPE] |= (1 << i);
								if (warn[SLOPE_WARN]) {
									sprintf (text, gmtdefs.d_format, gradient);
									sprintf (buffer, "%s - excessive %s gradient %s\n", placeStr, mgd77defs[i].abbrev, text);
									GMT_fputs (buffer, GMT_stdout);
								}
							}


#ifdef HISTOGRAM_MODE
							/* Use this to print excessive slopes to stderr for tracking winning
							   cruises when running histogram scripts */
							if (fabs(gradient) > maxSlope[i]) {
								E[curr].flags[E77_SLOPE] |= (1 << i);
								fprintf (stderr, "%s - excessive %s gradient %f\n", placeStr, mgd77defs[i].abbrev, gradient);
							}
#endif
						}
					}
					else
						gradient = dvalue = ds = MGD77_NaN;

					if ((1 << i) & (MGD77_GEOPHYSICAL_BITS | MGD77_CORRECTION_BITS)) {
						if (!strcmp(display,"SLOPES") && (D[curr].keep_nav || report_raw))
							out[curr][nout++] = gradient;
						if (!strcmp(display,"DFDS") && (D[curr].keep_nav || report_raw)) {
							out[curr][nout++] = fabs(dvalue);
							if (GMT_is_dnan(dvalue))
								out[curr][nout++] = MGD77_NaN;
							else {
								if (!strcmp(derivative,"TIME"))
									out[curr][nout++] = dt;
								else
									out[curr][nout++] = ds;
							}
						}
					}
				}
			}

			/* CRUISE - GRID COMPARISON */
			if (n_grids > 0) {
				for (i = 0; i < n_grids; i++) {

					if (this_grid[i].g_pts < 2 || !strcmp(this_grid[i].abbrev,"nav"))
						continue;

					/* Fill output array with cruise/grid differences */
					if (!strcmp(display,"DIFFS") && (D[curr].keep_nav || report_raw)) {
						out[curr][0] = D[curr].number[MGD77_LATITUDE];
						out[curr][1] = D[curr].number[MGD77_LONGITUDE];
						out[curr][2] = distance[curr]*distance_factor;
						out[curr][3+i*3] = D[curr].number[this_grid[i].col];
						out[curr][4+i*3] = G[i][curr];
						out[curr][5+i*3] = D[curr].number[this_grid[i].col]-G[i][curr];
					}

					if (M.bit_pattern[0] & (1 << this_grid[i].col)) {
						/* Track min/max absolute difference between grid and cruise */
						if (fabs(diff[i][curr]) > fabs(MaxDiff[i])) {
							MaxDiff[i] = diff[i][curr];
							iMaxDiff[i] = (int)curr;
						}

						/* Compare cruise and grid data to find offset areas (i.e., extended loss of bottom tracking) */
						if (curr > 0) {

							/* Areas are estimated for each offset by summing discrete rectangles while the offset sign
							   stays the same */
							/* Search backward to find a non-empty record for the same field. */
							/* Hope it doesn't have to search too far */
							for (j = 1; GMT_is_dnan(D[curr-j].number[this_grid[i].col]) && curr-j > 0; j++)
								if (j > MGD77_MAX_SEARCH) break;
							lastLat = D[curr-j].number[MGD77_LATITUDE];
							lastLon = D[curr-j].number[MGD77_LONGITUDE];							
							/* Calculate ds & dt between current and last valid record */
							/* Note: ds may be different for each field */
							ds = GMT_great_circle_dist (lastLon, lastLat, thisLon, thisLat) * MGD77_DEG_TO_KM;
							dt = D[curr].time-E[curr].utc_offset - D[curr-j].time-E[curr-j].utc_offset;

							/* Calculate area of this offset */
							if (!GMT_is_dnan(diff[i][curr])) {
								thisArea = 0.5 * (diff[i][curr] + diff[i][curr-j]) * ds;
								prevOffsetSign[i] = offsetSign[i];
								offsetSign[i] = thisArea > 0;
							}
							else
								thisArea = MGD77_NaN;

							/* Accumulate area of total offset (Attempt to skip big gaps in data) */
							/* Different types of gaps to avoid */
							/* 1. Lose GPS - time stays consistent but ds gets huge - max ds test */
							/* 2. Ship stops logging and moves to another region then logs again - max dt test */
							/* 3. No recorded time - use max ds test */
							if (!GMT_is_dnan(thisArea) && offsetSign[i] == prevOffsetSign[i] && \
							   (ds < maxGap || maxGap == 0)) {
								offsetArea[i] += thisArea;
								offsetLength[i] += ds;
							}

							/* Analyze offset after it ends (sign switch) */
							if (offsetSign[i] != prevOffsetSign[i] ||  curr == nvalues - 1) {
								/* Flag the offset if significant */
								if (fabs(offsetArea[i]) > mgd77snifferdefs[this_grid[i].col].maxArea) {
									if (warn[GRID_WARN]) {
										sprintf (text, gmtdefs.d_format, fabs(offsetArea[i]));
										sprintf (buffer, "%s - excessive offset from %s grid: Area/Length/Height %s\t",placeStr,this_grid[i].abbrev,text);
										GMT_fputs (buffer, GMT_stdout);
										sprintf (text, gmtdefs.d_format, offsetLength[i]);										
										sprintf (buffer, "%s\t",text);	GMT_fputs (buffer, GMT_stdout);
										sprintf (text, gmtdefs.d_format, offsetLength[i]);										
										sprintf (text, gmtdefs.d_format, fabs(offsetArea[i]/offsetLength[i]));
										sprintf (buffer, "%s\n",text);	GMT_fputs (buffer, GMT_stdout);
									}
									if (!strcmp(display,"E77"))
										fprintf (fpout, "%c-%c-%s-%s-%.2d: Extended offset from grid [%ld-%ld]\n",E77_REVIEW,E77_ERROR,list[argno],\
										this_grid[i].abbrev,E77_HDR_GRID_OFFSET,offsetStart[i]+1,curr+1);
									else if (warn[SUMMARY_WARN]) {
										sprintf (buffer, "%s (%s) extended offset from grid (%ld-%ld)\n",list[argno],this_grid[i].abbrev,\
										offsetStart[i]+1,curr+1);
										GMT_fputs (buffer, GMT_stdout);
									}
								}
								offsetArea[i] = 0;
								offsetLength[i] = 0;
								offsetStart[i] = curr;
							}
						}
					}
				}
			}

			/* Check for lower precision values and duplicate records */
			for (i = MGD77_LATITUDE; i < MGD77_N_NUMBER_FIELDS; i++) {

				/* Only check floating point fields present in this cruise */						
				if (M.bit_pattern[0] & (1 << i) & MGD77_FLOAT_BITS) {

					/* Compute the difference between current and previous value */
					if (!GMT_is_dnan(D[curr].number[i])) {

						/* Check for lower precision */
						if (curr > 0) {
							for (j = 1; GMT_is_dnan(D[curr-j].number[i]) && curr-j > 0; j++)
								if (j > MGD77_MAX_SEARCH) break;
							dvalue = D[curr].number[i]-D[curr-j].number[i];

							if (!GMT_is_dnan(dvalue) && dvalue != 0) {
								if (fmod(dvalue,1.0) != 0.0 && (lowPrecision & (1 << i)))
									lowPrecision -= (1 << i);
								if (fmod(dvalue,5.0) != 0.0 && (lowPrecision5 & (1 << i)))
									lowPrecision5 -= (1 << i);
							}

							/* Check for duplicate records */
							if (i == MGD77_MSD) continue;
							if (dvalue == 0)
								duplicates[i]++;
							else {
								if (duplicates[i] > MGD77_MAX_DUPLICATES) {
									if (warn[VALUE_WARN]) {
										sprintf (buffer, "%s - %d duplicate %s records\n",placeStr,\
										duplicates[i], mgd77defs[i].abbrev);
										GMT_fputs (buffer, GMT_stdout);
									}
									for (k = curr-1; k >= curr-duplicates[i]; k--)
										E[k].flags[E77_VALUE] |= (1 << i);
								}
								duplicates[i] = 0;
							}
						}
					}
				}
			}
#ifndef FIX
			/* Dump data row-by-row if requested */
			if (strcmp( display, "E77") && strcmp(display,"")) {
				if (!strcmp(display,"MGD77"))
					MGD77_Write_Data_Record_m77 (&Out, &D[curr]);
				else {
					if (curr > 0 || !strcmp(display,"VALS")) {
						if (GMT_io.binary[GMT_OUT])
							/* Use GMT output machinery which can handle binary output */
							GMT_output (GMT_stdout, n_out_columns, out[curr]);
						else {
							for (i = 0; i < (n_out_columns); i++) {
								GMT_ascii_output_one (GMT_stdout, out[curr][i], (int)i);
								if ((i+1) < n_out_columns) GMT_fputs (gmtdefs.field_delimiter, GMT_stdout);
							}
							GMT_fputs ("\n", GMT_stdout);
						}
					}
				}
			}
#endif
		}

#ifdef FIX
		/* Turn off fields if errors found */		
		for (rec = 0; rec < curr; rec++) {
			deleteRecord = FALSE;
			for (type = 0; type < N_ERROR_CLASSES; type++) {
				if (E[rec].flags[type]) { /*Error in this category */
					thisLon = D[rec].number[MGD77_LONGITUDE];
					thisLat = D[rec].number[MGD77_LATITUDE];
					switch (type) {
						case E77_NAV:
							/* 9-fill records with nav errors */
							for (i=MGD77_PTC; i<MGD77_N_NUMBER_FIELDS; i++) D[rec].number[i] = MGD77_NaN;
							deleteRecord = TRUE;
							break;
						default:
							for (field = MGD77_PTC; field < n_types[type]; field++) {
								if (E[rec].flags[type] & (1 << field))
									D[rec].number[field] = MGD77_NaN;
							}
							break;
					}
					D[rec].number[MGD77_LONGITUDE] = thisLon;
					D[rec].number[MGD77_LATITUDE] = thisLat;
				}
			}
			
			/* Dump "fixed" data row-by-row if requested */
			if (display != "E77" && strcmp(display,"") && !deleteRecord) {
				if (!strcmp(display,"MGD77"))
					MGD77_Write_Data_Record_m77 (&Out, &D[rec]);
				else {
					if (rec > 0 || !strcmp(display,"VALS")) {
						if (GMT_io.binary[GMT_OUT])
							/* Use GMT output machinery which can handle binary output */
							GMT_output (GMT_stdout, n_out_columns, out);
						else {
							for (i = 0; i < (n_out_columns); i++) {
								GMT_ascii_output_one (GMT_stdout, out[rec][i], i);
								if ((i+1) < n_out_columns) GMT_fputs (gmtdefs.field_delimiter, GMT_stdout);
							}
							GMT_fputs ("\n", GMT_stdout);
						}
					}
				}
			}
		}
#endif
		/*if (display && display != "E77") GMT_free ((void *)out[curr]);*/

		/* Test for PDR wrap */
		if (n_wrap > 0) {
			if (!strcmp(display,"E77"))
				fprintf (fpout, "%c-%c-%s-%s-%.2d: Encountered possible PDR wrap errors (%.1f) [%.1f]\n",E77_REVIEW,E77_ERROR,list[argno],\
				mgd77defs[MGD77_TWT].abbrev,E77_HDR_PDR, wrapsum/n_wrap, 5.0 * rint ((wrapsum/n_wrap)/5.0));
			else if (warn[SUMMARY_WARN]) {
				sprintf (buffer, "%s (%s) encountered possible PDR wrap errors (%.1f) [%.1f]\n",list[argno],mgd77defs[MGD77_TWT].abbrev,\
				wrapsum/n_wrap, 5.0 * rint ((wrapsum/n_wrap)/5.0));
				GMT_fputs (buffer, GMT_stdout);
			}
		}

		/* Bathymetry correction code warnings */
		if (warn[TYPE_WARN]) {
			if (bccCode != 63 && bccCode != 88 && (M.bit_pattern[0] & MGD77_TWT_BIT)) {
				sprintf (buffer, "%s - new bathymetry correction table available\n",placeStr);
				GMT_fputs (buffer, GMT_stdout);
			}
			else if (bccCode > 0 && bccCode < 56 && !(M.bit_pattern[0] & MGD77_TWT_BIT)) {
				sprintf (buffer, "%s - twt may be extracted from depth for Carter correction\n",placeStr);
				GMT_fputs (buffer, GMT_stdout);
			}
		}
		
		/* Warn if all values are zero, positive, or negative */
		if (warn[SUMMARY_WARN] || !strcmp(display,"E77")) {
			for (i = MGD77_MAG; i <= MGD77_FAA; i+=((int)MGD77_FAA-(int)MGD77_MAG)) {
				if (i == MGD77_MAG || i == MGD77_FAA) { /* Only check mag and faa for constant sign */
					if (warn[SUMMARY_WARN]) {
						if (MGD77_sign_bit[i] == (MGD77_ZERO_BIT)) {
							sprintf (buffer, "%s - all %s anomalies are zero.\n",placeStr,mgd77defs[i].abbrev);
							GMT_fputs (buffer, GMT_stdout);
						}
						else if (MGD77_sign_bit[i] == (MGD77_NEG_BIT)) {
							sprintf (buffer, "%s - only found negative %s values.\n",placeStr,mgd77defs[i].abbrev);
							GMT_fputs (buffer, GMT_stdout);
						}
						else if (MGD77_sign_bit[i] == (MGD77_NEG_BIT + MGD77_ZERO_BIT)) {
							sprintf (buffer, "%s - all %s values less than or equal to zero.\n",placeStr,mgd77defs[i].abbrev);
							GMT_fputs (buffer, GMT_stdout);
						}
						else if (MGD77_sign_bit[i] == (MGD77_POS_BIT)) {
							sprintf (buffer, "%s - only found positive %s values.\n",placeStr,mgd77defs[i].abbrev);
							GMT_fputs (buffer, GMT_stdout);
						}
						else if (MGD77_sign_bit[i] == (MGD77_POS_BIT + MGD77_ZERO_BIT)) {
							sprintf (buffer, "%s - all %s values greater than or equal to zero.\n",placeStr,mgd77defs[i].abbrev);
							GMT_fputs (buffer, GMT_stdout);
						}
					}
					if (!strcmp(display,"E77")) {
						if (MGD77_sign_bit[i] == (MGD77_ZERO_BIT))
							fprintf (fpout, "%c-%c-%s-%s-%.02d: all anomalies are zero\n",E77_APPLY,E77_WARN,list[argno],\
							mgd77defs[i].abbrev,E77_HDR_SIGN);						
						else if (MGD77_sign_bit[i] == (MGD77_NEG_BIT))
							fprintf (fpout, "%c-%c-%s-%s-%.02d: only found negative values\n",E77_APPLY,E77_WARN,list[argno],\
							mgd77defs[i].abbrev,E77_HDR_SIGN);						
						else if (MGD77_sign_bit[i] == (MGD77_NEG_BIT + MGD77_ZERO_BIT))						
							fprintf (fpout, "%c-%c-%s-%s-%.02d: all values less than or equal to zero\n",E77_APPLY,E77_WARN,list[argno],\
							mgd77defs[i].abbrev,E77_HDR_SIGN);						
						else if (MGD77_sign_bit[i] == (MGD77_POS_BIT))
							fprintf (fpout, "%c-%c-%s-%s-%.02d: only found positive values\n",E77_APPLY,E77_WARN,list[argno],\
							mgd77defs[i].abbrev,E77_HDR_SIGN);						
						else if (MGD77_sign_bit[i] == (MGD77_POS_BIT + MGD77_ZERO_BIT))
							fprintf (fpout, "%c-%c-%s-%s-%.02d: all values greater than or equal to zero\n",E77_APPLY,E77_WARN,list[argno],\
							mgd77defs[i].abbrev,E77_HDR_SIGN);
					}
				}
			}
		}

		/* OUTPUT E77 ERROR FORMAT */
		if (!strcmp(display,"E77")) {
			if (gmtdefs.verbose == 2)
				fprintf (stderr, "%s: Generating errata table %s.e77\n", GMT_program,M.NGDC_id);
			/* Echo out the user-specified invalid data records */
			for (i = 0; i < (int)n_bad_sections; i++) {
				fprintf (fpout, "%c-%c-%s-%s-%.02d: Invalid data records: [%ld-%ld]\n",E77_APPLY,E77_ERROR,list[argno],\
				BadSection[i].abbrev,E77_HDR_FLAGRANGE,BadSection[i].start,BadSection[i].stop);
			}

			/* E77 HEADER MESSAGES */
			if ((bccCode != 63 && bccCode != 88 && (M.bit_pattern[0] & MGD77_TWT_BIT)) OR_TRUE)
				fprintf (fpout, "%c-%c-%s-twt-%.2d: More recent bathymetry correction table available\n",E77_APPLY,E77_WARN,list[argno],E77_HDR_BCC);
			if ((bccCode > 0 && bccCode < 56 && !(M.bit_pattern[0] & MGD77_TWT_BIT)) OR_TRUE)
				fprintf (fpout, "%c-%c-%s-twt-%.2d: twt may be extracted from depth for Carter correction\n",E77_APPLY,E77_WARN,list[argno],E77_HDR_BCC);

			/* Output data precision warnings */
			if (lowPrecision || lowPrecision5 OR_TRUE) {
				for (k = MGD77_LATITUDE; k < MGD77_N_NUMBER_FIELDS; k++) {
					if (lowPrecision5 & (1 << k) OR_TRUE)
						fprintf(fpout, "%c-%c-%s-%s-%.02d: Integer multiple of 5 precision\n",E77_APPLY,E77_WARN,list[argno],mgd77defs[k].abbrev,\
						E77_HDR_PRECISION);
					if (!(lowPrecision5 & (1 << k) OR_TRUE) && lowPrecision & (1 << k) OR_TRUE)
						fprintf(fpout, "%c-%c-%s-%s-%.02d: Integer precision\n",E77_APPLY,E77_WARN,list[argno],mgd77defs[k].abbrev,E77_HDR_PRECISION);
				}
			}

			/* E77 ERROR RECORDS */
			fprintf (fpout,"# Errata: Data\n");
			for (rec = 0; rec < curr; rec++) {
				sprintf (placeStr, "%s%s%s%s%ld%s",list[argno],gmtdefs.field_delimiter,timeStr,gmtdefs.field_delimiter,rec+1,\
				gmtdefs.field_delimiter);
				errorStr[0]='\0';
				for (type = 0; type < N_ERROR_CLASSES; type++) {
					if (E[rec].flags[type] OR_TRUE) { /*Error in this category */
						for (field = 0; field < (int)n_types[type]; field++) {
							if (E[rec].flags[type] & (1 << field) OR_TRUE)
								sprintf (errorStr, "%s%c", errorStr, (int)('A'+field));
						}
					}
					else
						sprintf (errorStr, "%s0",errorStr);
					if (type < N_ERROR_CLASSES-1)
						sprintf (errorStr, "%s-",errorStr);
				}
				if (!strcmp(errorStr,"0-0-0")) continue;
				if (gotTime)
					GMT_ascii_format_one (timeStr, D[rec].time, GMT_io.out_col_type[MGD77_TIME]);
				else
					GMT_ascii_format_one (timeStr, distance[rec], GMT_io.out_col_type[GMT_IS_FLOAT]);
				/* Version 1 data corrections apply crucial nav errors and not value and gradient errors */
				sprintf (placeStr, "%s%s%s%s%ld%s",list[argno],gmtdefs.field_delimiter,timeStr,gmtdefs.field_delimiter,rec+1,\
				gmtdefs.field_delimiter);
				if ((!D[rec].keep_nav && E[rec].flags[E77_NAV]) || E[rec].utc_offset OR_TRUE)
					sprintf (placeStr, "%c%s%s%s%s%s%ld%s",E77_APPLY,gmtdefs.field_delimiter,list[argno],gmtdefs.field_delimiter,\
					timeStr,gmtdefs.field_delimiter,rec+1,gmtdefs.field_delimiter);
				else
					sprintf (placeStr, "%c%s%s%s%s%s%ld%s",E77_REVIEW,gmtdefs.field_delimiter,list[argno],gmtdefs.field_delimiter,\
					timeStr,gmtdefs.field_delimiter,rec+1,gmtdefs.field_delimiter);
				fprintf (fpout, "%s%s%s",placeStr,errorStr,gmtdefs.field_delimiter);
				prevType = FALSE;
				for (type = 0; type < N_ERROR_CLASSES; type++) {
					if (E[rec].flags[type] OR_TRUE) { /*Error in this category */
						fprintf (fpout, " ");
						if (prevType && (E[rec].flags[type] OR_TRUE))
							fprintf(fpout, "- ");
						if (type == E77_NAV)
							fprintf (fpout, "NAV: ");
						if (type == E77_VALUE)
							fprintf (fpout, "VAL: ");
						if (type == E77_SLOPE)
							fprintf (fpout, "GRAD: ");
						for (field=0; field < (int)n_types[type]; field++) {
							if (E[rec].flags[type] & (1 << field) OR_TRUE) {
								if (prevFlag)
									fprintf (fpout, ", ");
								switch (type) {
									case E77_NAV:
										switch (1 << field) {
											case NAV_TIME_OOR:
												fprintf (fpout, "time out of range");
												break;
											case NAV_TIME_NONINC:
												fprintf (fpout, "non-increasing time");
												break;
											case NAV_HISPD:
												fprintf (fpout, "excessive speed");
												break;
											case NAV_ON_LAND:
												fprintf (fpout, "on land");
												break;
											case NAV_UNDEF:
												fprintf (fpout, "navigation undefined");
												break;
											case NAV_TZ_ERROR:
												fprintf (fpout, "UTC shifted %d hr by time zone crossing error",E[rec].utc_offset/3600);
												break;
											default:
												fprintf (fpout, "undefined nav error!");
												break;
										}
										break;
									default:
										fprintf (fpout, "%s",mgd77defs[field].abbrev);
										break;
								}
								prevFlag = TRUE;
							}
						}
						prevFlag = FALSE;
						switch (type) {
							case E77_VALUE:
								fprintf (fpout, " invalid");
								break;
							case E77_SLOPE:
								fprintf (fpout, " excessive");
								break;
							default:
								break;
						}
						prevType = TRUE;					
					}
				}
				fprintf (fpout, "\n");
			}
		}

		/* Print Error Summary */
		if (warn[SUMMARY_WARN]) {
			if (noTimeCount == curr - 1) {			/* no valid time in data */
				sprintf (buffer, "%s (time) Cruise contains no time record\n", list[argno]);
				GMT_fputs (buffer, GMT_stdout);
			}

			if (noTimeCount >0 && noTimeCount < curr) {	/* print the first and number of nine-filled time records */
				sprintf (buffer, "%s (time) %d records contain no time starting at record #%d\n",list[argno],\
				noTimeCount, noTimeStart);
				GMT_fputs (buffer, GMT_stdout);
			}

			if (timeErrorCount > 0 && timeErrorCount < curr) {/* print the first and number of time errors */
				sprintf (buffer, "%s (time) %d records had time errors starting at record #%d\n",list[argno],\
				timeErrorCount, timeErrorStart);
				GMT_fputs (buffer, GMT_stdout);
			}

			if (distanceErrorCount > 0) { /* print the first and number of distance errors */
				sprintf (buffer, "%s (dist) %d records had distance errors starting at record #%d\n",list[argno],\
				distanceErrorCount,distanceErrorStart);
				GMT_fputs (buffer, GMT_stdout);
			}

			for (i = MGD77_LATITUDE; i < MGD77_N_NUMBER_FIELDS; i++) {
				if ((lowPrecision & (1 << i)) && !(lowPrecision5 & (1 << i))) {
					sprintf (buffer, "%s (%s) Data Precision Warning: only integer values found\n",list[argno],mgd77defs[i].abbrev);
					GMT_fputs (buffer, GMT_stdout);
				}
				if (lowPrecision5 & (1 << i)) {	/* low precision data */
					sprintf (buffer, "%s (%s) Data Precision Warning: only integer multiples of 5 found\n",list[argno], mgd77defs[i].abbrev);
					GMT_fputs (buffer, GMT_stdout);
				}
			}
			
			if (n_grids) {
				for (i = 0; i < n_grids; i++) {
					sprintf (text, gmtdefs.d_format, MaxDiff[i]);
					sprintf (buffer, "%s (%s) Max ship-grid difference [%s] at record %d\n",list[argno],\
					mgd77defs[this_grid[i].col].abbrev,text,iMaxDiff[i]);
					GMT_fputs (buffer, GMT_stdout);
				}
			}
			
			if (landcruise) {	/* vessel went over land (GPS error or other gross navigation) */
				sprintf (buffer, "%s (nav) Navigation Warning: %d records went over land starting at record %d\n",\
				list[argno], overLandCount, overLandStart);
				GMT_fputs (buffer, GMT_stdout);
			}
		}
		/* Clean-up after finishing this cruise */
		MGD77_Close_File (&M);
		GMT_free ((void *)D);
/*		GMT_free ((void *)offsetArea);
		GMT_free ((void *)offsetStart);
		GMT_free ((void *)offsetLength);
		GMT_free ((void *)offsetSign);
		GMT_free ((void *)prevOffsetSign);*/
		GMT_free ((void *)distance);
		GMT_free ((void *)out);
		MGD77_Free_Header_Record (&Out, &H);
		if (n_grids > 0) {
			for (i = 0; i < n_grids; i++) {
				GMT_free ((void *)G[i]);
				GMT_free ((void *)diff[i]);
			}
			GMT_free ((void *)G);
			GMT_free ((void *)diff);
		}
		if (!strcmp(display,"E77"))
			fclose (fpout);
	}
	GMT_free ((void *)E);
	/* De-allocate grid memory */
	if (n_grids > 0) {
		GMT_free ((void *)MaxDiff);
		GMT_free ((void *)iMaxDiff);
		for (i = 0; i < n_grids; i++)
			GMT_free ((void *)f[i]);
	}

	MGD77_Path_Free (n_paths, list);
	MGD77_end (&M);
#ifdef DEBUG
	GMT_memtrack_on (GMT_mem_keeper);
#endif

	exit (EXIT_SUCCESS);
}

void regress_rls (double *x, double *y, GMT_LONG nvalues, double *stat, int col)
{
	GMT_LONG i, n;
	double y_hat, threshold, s_0, res, *xx, *yy, corr=0.0;
	
	regress_lms (x, y, nvalues, stat, col);
	/* Get LMS scale and use 2.5 of it to detect regression outliers */
	s_0 = 1.4826 * (1.0 + 5.0 / nvalues) * sqrt (stat[MGD77_RLS_STD]);
	threshold = 2.5 * s_0;
	
	xx = (double *)GMT_memory (VNULL, (size_t)nvalues, sizeof (double), "mgd77sniffer");
	yy = (double *)GMT_memory (VNULL, (size_t)nvalues, sizeof (double), "mgd77sniffer");
	for (i = n = 0; i < nvalues; i++) {
		y_hat = stat[MGD77_RLS_SLOPE] * x[i] + stat[MGD77_RLS_ICEPT];
		res = y[i] - y_hat;
		if (fabs (res) > threshold) continue;	/* Skip outliers */
		xx[n] = x[i];
		yy[n] = y[i];
		n++;
	}
	/* Now do LS regression on the 'good' points */
	regress_ls (xx, yy, n, stat, col);
	/*stat[MGD77_RLS_CORR] = GMT_corrcoeff (xx, yy, n, 0);*/
	corr=stat[MGD77_RLS_CORR];
	if (stat[MGD77_RLS_CORR] == 1.0) corr=stat[MGD77_RLS_CORR]-FLT_EPSILON;
	if (n > 2) {	/* Determine if correlation is significant at 95% */
		double t, tcrit;
		t = corr * sqrt (n - 2.0) / sqrt (1.0 - corr * corr);
		tcrit = GMT_tcrit (0.95, (double)n - 2.0);
		stat[MGD77_RLS_SIG] = (double)(t > tcrit);	/* 1.0 if significant, 0.0 otherwise */
	}
	else
		stat[MGD77_RLS_SIG] = GMT_d_NaN;

	GMT_free ((void *)xx);
	GMT_free ((void *)yy);
}

void regress_ls (double *x, double *y, GMT_LONG n, double *stat, int col)
{
	GMT_LONG i;
	double sum_x, sum_y, sum_x2, sum_y2, sum_xy, d, ss;
	double mean_x, mean_y, S_xx, S_xy, S_yy, y_discrepancy;
	
	sum_x = sum_y = sum_x2 = sum_y2 = sum_xy = y_discrepancy = 0.0;
	mean_x = mean_y = S_xx = S_xy = S_yy = ss = 0.0;
	for (i = 0; i < n; i++) {
		sum_x += x[i];
		sum_y += y[i];
		sum_x2 += x[i] * x[i];
		sum_y2 += y[i] * y[i];
		sum_xy += x[i] * y[i];
		ss += (x[i] - y[i])*(x[i] - y[i]);        /* sum of squared residuals */
	}
	
	mean_x = sum_x / n;
	mean_y = sum_y / n;
	
	for (i = 0; i < n; i++) {
		S_xx += (x[i] - mean_x) * (x[i] - mean_x);
		S_yy += (y[i] - mean_y) * (y[i] - mean_y);
		S_xy += (x[i] - mean_x) * (y[i] - mean_y);
	}

/*	S_xy = sum_xy - n * mean_x * mean_y;
	S_xx = sum_x2 - n * mean_x * mean_x;
	S_yy = sum_y2 - n * mean_y * mean_y; */
	if (col != MGD77_DEPTH) { /* Use LMS m & b for depth (since offset forced to 0) */
		stat[MGD77_RLS_SLOPE] = S_xy / S_xx;                                    /* Slope */
		stat[MGD77_RLS_ICEPT] = mean_y - stat[MGD77_RLS_SLOPE] * mean_x;        /* Intercept */
	}
	
	for (i = 0; i < n; i++) {
		d = y[i] - stat[MGD77_RLS_SLOPE] * x[i] - stat[MGD77_RLS_ICEPT];	
		y_discrepancy += d*d;
	}
	stat[MGD77_RLS_STD] = sqrt (y_discrepancy / (n-1));         /* Standard deviation */
	stat[MGD77_RLS_SXX] = S_xx;                                 /* Sum of squares */
	stat[MGD77_RLS_CORR] = sqrt(S_xy * S_xy / (S_xx * S_yy));   /* Correlation (r) */
	stat[MGD77_RLS_RMS] = sqrt(ss / n);                         /* rms */
	stat[MGD77_RLS_SUMX2] = sum_x2;                             /* Sum of x^2 */
}

void regress_lms (double *x, double *y, GMT_LONG nvalues, double *stat, int col)
{

	double d_angle, limit, a, old_error, d_error, angle_0, angle_1;
	int n_angle;

	d_angle = 1.0;
	limit = 0.1;
	n_angle = irint ((180.0 - 2 * d_angle) / d_angle) + 1;
	regresslms_sub (x, y, -90.0 + d_angle, 90.0 - d_angle, nvalues, n_angle, stat, col);
	old_error = stat[MGD77_RLS_STD];
	d_error = stat[MGD77_RLS_STD];

	while (d_error > limit) {
		d_angle = 0.1 * d_angle;
		a = atan (stat[MGD77_RLS_SLOPE]) * 180 / M_PI;
		angle_0 = floor (a / d_angle) * d_angle - d_angle;
		angle_1 = angle_0 + 2.0 * d_angle;
		regresslms_sub (x, y, angle_0, angle_1, nvalues, 21, stat, col);
		d_error = fabs (stat[MGD77_RLS_STD] - old_error);
		old_error = stat[MGD77_RLS_STD];
	}
}

void regresslms_sub (double *x, double *y, double angle0, double angle1, GMT_LONG nvalues, int n_angle, double *stat, int col)
{
	double da, *slp, *icept, *z, *sq_misfit, *angle, *e, emin = DBL_MAX, d;
	GMT_LONG i, j = 0;

	slp = (double *) GMT_memory (VNULL, (size_t) n_angle, sizeof (double), "mgd77sniffer");
	icept = (double *) GMT_memory (VNULL, (size_t) n_angle, sizeof (double), "mgd77sniffer");
	angle = (double *) GMT_memory (VNULL, (size_t) n_angle, sizeof (double), "mgd77sniffer");
	e = (double *) GMT_memory (VNULL, (size_t) n_angle, sizeof (double), "mgd77sniffer");
	z = (double *) GMT_memory (VNULL, (size_t) nvalues, sizeof (double), "mgd77sniffer");
	sq_misfit = (double *) GMT_memory (VNULL, (size_t) nvalues, sizeof (double), "mgd77sniffer");

	for (i=0; i < 4; i++)
		stat[i] = 0;
	memset ((void *)slp,   0, (size_t)(n_angle *sizeof (double)));
	memset ((void *)icept, 0, (size_t)(n_angle *sizeof (double)));
	memset ((void *)angle, 0, (size_t)(n_angle *sizeof (double)));
	memset ((void *)e,     0, (size_t)(n_angle *sizeof (double)));
	da = (angle1 - angle0) / (n_angle - 1);
	
	for (i = 0; i < n_angle; i++) {
		angle[i] = angle0 + i * da;
		slp[i] = tan (angle[i] * M_PI / 180.0);
		for (j = 0; j < nvalues; j++)
			z[j] = y[j] - slp[i] * x[j];
		if (col == MGD77_DEPTH)
			icept[i] = 0.0;
		else
			icept[i] = lms (z, nvalues);
		for (j = 0; j < nvalues; j++) {
			d = z[j]-icept[i];
			sq_misfit[j] = d*d;
		}
		e[i] = median (sq_misfit, nvalues);
	}
	for (i = 0; i < n_angle; i++) {
		if (e[i] < emin || i == 0) {
			emin = e[i];
			j = i;
		}
	}
	stat[MGD77_RLS_SLOPE] = slp[j];
	stat[MGD77_RLS_ICEPT] = icept[j];
	stat[MGD77_RLS_STD] = e[j];

	GMT_free ((void *)slp);
	GMT_free ((void *)icept);
	GMT_free ((void *)angle);
	GMT_free ((void *)e);
	GMT_free ((void *)z);
	GMT_free ((void *)sq_misfit);
}

double lms (double *x, GMT_LONG n)
{
	double mode;
	GMT_LONG GMT_n_multiples = 0;

	GMT_mode (x, n, n/2, 1, 0, &GMT_n_multiples, &mode);
	return mode;
}

double median (double *x, GMT_LONG n)
{
	double *sorted, med;
	
	sorted = (double *) GMT_memory (VNULL, (size_t) n, sizeof (double), "mgd77sniffer");
	memcpy ((void *)sorted, (void *)x, (size_t)(n *sizeof (double)));
	qsort ((void *) sorted, (size_t)n, sizeof(double), GMT_comp_double_asc);
	med = (n%2) ? sorted[n/2] : 0.5*(sorted[(n-1)/2]+sorted[n/2]);
	GMT_free ((void *)sorted);
	return med;
}

/* Read Grid Header (from Smith & Wessel grdtrack.c) */
void read_grid (struct MGD77_GRID_INFO *info, float **grid, double w, double e, double s, double n, GMT_LONG interpolant, double threshold) {

	if (strlen(info->fname) == 0) return;	/* No name */
	
	GMT_pad[0] = GMT_pad[1] = GMT_pad[2] = GMT_pad[3] = 2;  /* GMT_pad is declared automatically in gmt.h */
	
	if (info->format == 0) {	/* GMT geographic grid with header */
		GMT_err_fail (GMT_read_grd_info (info->fname, &info->grdhdr), info->fname);

		/* Get grid dimensions */
		info->one_or_zero = (info->grdhdr.node_offset) ? 0 : 1;
		info->nx = irint ( (info->grdhdr.x_max - info->grdhdr.x_min) / info->grdhdr.x_inc) + info->one_or_zero;
		info->ny = irint ( (info->grdhdr.y_max - info->grdhdr.y_min) / info->grdhdr.y_inc) + info->one_or_zero;

		/* Allocate grid memory */
		*grid = (float *) GMT_memory (VNULL, (size_t)((info->nx + 4) * (info->ny + 4)), sizeof (float), GMT_program);
		
		/* Read into memory with 2 rows/cols as padding */
		GMT_err_fail (GMT_read_grd (info->fname, &info->grdhdr, *grid, w, e, s, n, GMT_pad, FALSE), info->fname);
	}
	else {	/* Read a Mercator grid Sandwell/Smith style */
		GMT_read_img (info->fname, &info->grdhdr, grid, w, e, s, n, info->scale, info->mode, info->max_lat, TRUE);
	}

	info->mx = info->grdhdr.nx + 4;
	if (GMT_360_RANGE (info->grdhdr.x_max, info->grdhdr.x_min)) GMT_boundcond_parse (&info->edgeinfo, "g");
	
	GMT_boundcond_param_prep (&info->grdhdr, &info->edgeinfo);

	/* Initialize bcr structure with 2 row/col boundaries:  */

	GMT_bcr_init (&info->grdhdr, GMT_pad, interpolant, threshold, &info->bcr);
	
	/* Set boundary conditions  */
	GMT_boundcond_set (&info->grdhdr, &info->edgeinfo, GMT_pad, *grid);
}

/* Sample Grid at Cruise Locations (from Smith & Wessel grdtrack.c) */
int sample_grid (struct MGD77_GRID_INFO *info, struct MGD77_DATA_RECORD *D, double **g, float *grid, GMT_LONG n_grid, GMT_LONG n) {

	GMT_LONG rec, pts = 0;
	double MGD77_NaN, x, y;
	GMT_make_dnan (MGD77_NaN);

	/* Get grid values at cruise locations */
	for (rec = 0; rec < n; rec++) {

		if (info->format == 1)	{/* Mercator IMG grid - get Mercator coordinates x,y */
			GMT_geo_to_xy (D[rec].number[MGD77_LONGITUDE], D[rec].number[MGD77_LATITUDE], &x, &y);
			if (x > info->grdhdr.x_max) x -= 360.0;
		}
		else {		/* Regular geographic grd, just copy lon,lat to x,y */
			x = D[rec].number[MGD77_LONGITUDE];
			y = D[rec].number[MGD77_LATITUDE];
			/* Adjust cruise longitude if necessary; We know cruise is between +/-180 */
			if (info->grdhdr.x_min >= 0.0 && D[rec].number[MGD77_LONGITUDE] < 0.0)
				GMT_lon_range_adjust(0, &x);	/* Adjust to 0-360 range */
		}
		g[n_grid][rec] = MGD77_NaN;	/* Default value if we are outside the grid domain */
		if (y < info->grdhdr.y_min || y > info->grdhdr.y_max) continue;	/* Outside latitude range */

		if (GMT_io.in_col_type[0] == GMT_IS_LON) {
			while (x > info->grdhdr.x_max) x -= 360.0;
			while (x < info->grdhdr.x_min) x += 360.0;
		}

		/* If point is outside grd area, shift it using periodicity or skip if not periodic. */
		while ( (x < info->grdhdr.x_min) && (info->edgeinfo.nxp > 0)) x += (info->grdhdr.x_inc * info->edgeinfo.nxp);
		if (x < info->grdhdr.x_min) continue;  /* West of our area */

		while ((x > info->grdhdr.x_max) && (info->edgeinfo.nxp > 0)) x -= (info->grdhdr.x_inc * info->edgeinfo.nxp);
		if (x > info->grdhdr.x_max) continue;  /* East of our area */

		/* Get the value from the grid - it could be NaN */
		g[n_grid][rec] = GMT_get_bcr_z (&info->grdhdr, x, y, grid, &info->edgeinfo, &info->bcr);
		pts++;
	}
	return ((int)pts);
}

/* Decimation benefits marine gravity due to amplitude differences */
/* between ship and satellite data and also broadens confidence  */
/* intervals for any ship grid comparisons by reducing excessive */
/* number of degrees of freedom */
/* Then create arrays for passing to RLS */
GMT_LONG decimate (double *new, double *orig, GMT_LONG nclean, double min, double max, double delta, double **dec_new, double **dec_orig, GMT_LONG *extreme, char *fieldTest) {

	GMT_LONG n, j, k, npts, ship_bin, grid_bin;
	int **bin2d;
	double *dorig, *dnew;
#ifdef DUMP_DECIMATE
	char buffer[BUFSIZ];
#endif

	/* Create a 2-D bin table */
	n = irint ((max - min)/delta) + 1;
	bin2d = (int **)GMT_memory (VNULL, (size_t)n, sizeof (int *), GMT_program);
	for (j = 0; j < n; j++)
		bin2d[j] = (int *)GMT_memory (VNULL, (size_t)n,  sizeof (int), GMT_program);

	/* Then loop over all the ship, cruise pairs */
	*extreme=0;
	for (j = 0; j < nclean; j++) {
		/* Need to skip ship and grid values that are outside of acceptable range */
		if (orig[j] >= min && orig[j] <= max && new[j] >= min && new[j] <= max) {
			ship_bin = irint ((orig[j] - min)/delta);
			grid_bin = irint ((new[j] - min)/delta);
			bin2d[ship_bin][grid_bin]++;    /* Add up # of pairs in this bin */
		}
		else *extreme=*extreme+1;
	}
		
	/* Then find how many binned pairs we got */
	for (ship_bin = npts = 0; ship_bin < n; ship_bin++) {
		for (grid_bin = 0; grid_bin < n; grid_bin++) {
			if (bin2d[ship_bin][grid_bin] > 0)
				npts++;
		}
	}

	dorig = (double *)GMT_memory (VNULL, (size_t)npts, sizeof (double), GMT_program);
	dnew = (double *)GMT_memory (VNULL, (size_t)npts, sizeof (double), GMT_program);

	for (ship_bin = k = 0; ship_bin < n; ship_bin ++) {
		for (grid_bin = 0; grid_bin < n; grid_bin ++) {
			if (bin2d[ship_bin][grid_bin]) {
				dorig[k] = min + ship_bin * delta;
				dnew[k] = min + grid_bin * delta;
#ifdef DUMP_DECIMATE
				sprintf(buffer,"%s\torig:\t%f\tnew:\t%f\n",fieldTest,dorig[k],dnew[k]);
				GMT_fputs (buffer, GMT_stdout);
#endif
				k++;	/* Count number of non-empty bins */
			}
		}
	}
	
	*dec_orig = dorig;
	*dec_new = dnew;
	
	for (j = 0; j < n; j++)	GMT_free ((void *)bin2d[j]);
	GMT_free ((void *)bin2d);					
	return npts;
}
