/*--------------------------------------------------------------------
 *
 *	gmtmath_def.h [Generated by make_math.sh]
 *
 *	Copyright (c) 1991-2015 by P. Wessel and W. H. F. Smith
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
 *	Contact info: gmt.soest.hawaii.edu
 *--------------------------------------------------------------------
 *	gmtmath_def.h is automatically generated by make_math.sh;
 *	Do NOT edit manually!
 */

#define GMTMATH_N_OPERATORS	131

/* For backward compatibility: */

#define ADD	5
#define DIV	32
#define MUL	82
#define POW	92
#define SUB	116

/* Declare all functions to return void */

void table_ABS(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 0 */
void table_ACOS(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 1 */
void table_ACOSH(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 2 */
void table_ACOT(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 3 */
void table_ACSC(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 4 */
void table_ADD(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 5 */
void table_AND(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 6 */
void table_ASEC(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 7 */
void table_ASIN(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 8 */
void table_ASINH(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 9 */
void table_ATAN(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 10 */
void table_ATAN2(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 11 */
void table_ATANH(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 12 */
void table_BEI(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 13 */
void table_BER(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 14 */
void table_CEIL(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 15 */
void table_CHICRIT(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 16 */
void table_CHIDIST(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 17 */
void table_COL(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 18 */
void table_CORRCOEFF(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 19 */
void table_COS(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 20 */
void table_COSD(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 21 */
void table_COSH(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 22 */
void table_COT(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 23 */
void table_COTD(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 24 */
void table_CPOISS(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 25 */
void table_CSC(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 26 */
void table_CSCD(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 27 */
void table_D2DT2(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 28 */
void table_D2R(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 29 */
void table_DDT(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 30 */
void table_DILOG(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 31 */
void table_DIV(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 32 */
void table_DUP(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 33 */
void table_EQ(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 34 */
void table_ERF(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 35 */
void table_ERFC(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 36 */
void table_ERFINV(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 37 */
void table_EXCH(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 38 */
void table_EXP(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 39 */
void table_FACT(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 40 */
void table_FCRIT(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 41 */
void table_FDIST(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 42 */
void table_FLIPUD(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 43 */
void table_FLOOR(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 44 */
void table_FMOD(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 45 */
void table_GE(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 46 */
void table_GT(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 47 */
void table_HYPOT(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 48 */
void table_I0(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 49 */
void table_I1(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 50 */
void table_IN(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 51 */
void table_INRANGE(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 52 */
void table_INT(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 53 */
void table_INV(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 54 */
void table_ISNAN(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 55 */
void table_J0(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 56 */
void table_J1(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 57 */
void table_JN(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 58 */
void table_K0(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 59 */
void table_K1(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 60 */
void table_KEI(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 61 */
void table_KER(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 62 */
void table_KN(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 63 */
void table_KURT(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 64 */
void table_LE(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 65 */
void table_LMSSCL(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 66 */
void table_LOG(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 67 */
void table_LOG10(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 68 */
void table_LOG1P(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 69 */
void table_LOG2(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 70 */
void table_LOWER(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 71 */
void table_LRAND(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 72 */
void table_LSQFIT(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 73 */
void table_LT(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 74 */
void table_MAD(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 75 */
void table_MAX(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 76 */
void table_MEAN(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 77 */
void table_MED(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 78 */
void table_MIN(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 79 */
void table_MOD(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 80 */
void table_MODE(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 81 */
void table_MUL(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 82 */
void table_NAN(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 83 */
void table_NEG(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 84 */
void table_NEQ(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 85 */
void table_NOT(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 86 */
void table_NRAND(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 87 */
void table_OR(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 88 */
void table_PLM(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 89 */
void table_PLMg(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 90 */
void table_POP(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 91 */
void table_POW(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 92 */
void table_PQUANT(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 93 */
void table_PSI(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 94 */
void table_PV(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 95 */
void table_QV(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 96 */
void table_R2(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 97 */
void table_R2D(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 98 */
void table_RAND(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 99 */
void table_RINT(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 100 */
void table_ROOTS(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 101 */
void table_ROTT(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 102 */
void table_SEC(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 103 */
void table_SECD(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 104 */
void table_SIGN(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 105 */
void table_SIN(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 106 */
void table_SINC(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 107 */
void table_SIND(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 108 */
void table_SINH(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 109 */
void table_SKEW(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 110 */
void table_SQR(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 111 */
void table_SQRT(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 112 */
void table_STD(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 113 */
void table_STEP(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 114 */
void table_STEPT(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 115 */
void table_SUB(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 116 */
void table_SUM(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 117 */
void table_TAN(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 118 */
void table_TAND(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 119 */
void table_TANH(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 120 */
void table_TCRIT(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 121 */
void table_TDIST(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 122 */
void table_TN(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 123 */
void table_UPPER(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 124 */
void table_XOR(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 125 */
void table_Y0(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 126 */
void table_Y1(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 127 */
void table_YN(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 128 */
void table_ZCRIT(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 129 */
void table_ZDIST(struct GMTMATH_INFO *info, double **stack[], GMT_LONG *constant, double *factor, GMT_LONG last, GMT_LONG start, GMT_LONG n);		/* id = 130 */

/* Declare operator array */

char *operator[GMTMATH_N_OPERATORS] = {
	"ABS",		/* id = 0 */
	"ACOS",		/* id = 1 */
	"ACOSH",		/* id = 2 */
	"ACOT",		/* id = 3 */
	"ACSC",		/* id = 4 */
	"ADD",		/* id = 5 */
	"AND",		/* id = 6 */
	"ASEC",		/* id = 7 */
	"ASIN",		/* id = 8 */
	"ASINH",		/* id = 9 */
	"ATAN",		/* id = 10 */
	"ATAN2",		/* id = 11 */
	"ATANH",		/* id = 12 */
	"BEI",		/* id = 13 */
	"BER",		/* id = 14 */
	"CEIL",		/* id = 15 */
	"CHICRIT",		/* id = 16 */
	"CHIDIST",		/* id = 17 */
	"COL",		/* id = 18 */
	"CORRCOEFF",		/* id = 19 */
	"COS",		/* id = 20 */
	"COSD",		/* id = 21 */
	"COSH",		/* id = 22 */
	"COT",		/* id = 23 */
	"COTD",		/* id = 24 */
	"CPOISS",		/* id = 25 */
	"CSC",		/* id = 26 */
	"CSCD",		/* id = 27 */
	"D2DT2",		/* id = 28 */
	"D2R",		/* id = 29 */
	"DDT",		/* id = 30 */
	"DILOG",		/* id = 31 */
	"DIV",		/* id = 32 */
	"DUP",		/* id = 33 */
	"EQ",		/* id = 34 */
	"ERF",		/* id = 35 */
	"ERFC",		/* id = 36 */
	"ERFINV",		/* id = 37 */
	"EXCH",		/* id = 38 */
	"EXP",		/* id = 39 */
	"FACT",		/* id = 40 */
	"FCRIT",		/* id = 41 */
	"FDIST",		/* id = 42 */
	"FLIPUD",		/* id = 43 */
	"FLOOR",		/* id = 44 */
	"FMOD",		/* id = 45 */
	"GE",		/* id = 46 */
	"GT",		/* id = 47 */
	"HYPOT",		/* id = 48 */
	"I0",		/* id = 49 */
	"I1",		/* id = 50 */
	"IN",		/* id = 51 */
	"INRANGE",		/* id = 52 */
	"INT",		/* id = 53 */
	"INV",		/* id = 54 */
	"ISNAN",		/* id = 55 */
	"J0",		/* id = 56 */
	"J1",		/* id = 57 */
	"JN",		/* id = 58 */
	"K0",		/* id = 59 */
	"K1",		/* id = 60 */
	"KEI",		/* id = 61 */
	"KER",		/* id = 62 */
	"KN",		/* id = 63 */
	"KURT",		/* id = 64 */
	"LE",		/* id = 65 */
	"LMSSCL",		/* id = 66 */
	"LOG",		/* id = 67 */
	"LOG10",		/* id = 68 */
	"LOG1P",		/* id = 69 */
	"LOG2",		/* id = 70 */
	"LOWER",		/* id = 71 */
	"LRAND",		/* id = 72 */
	"LSQFIT",		/* id = 73 */
	"LT",		/* id = 74 */
	"MAD",		/* id = 75 */
	"MAX",		/* id = 76 */
	"MEAN",		/* id = 77 */
	"MED",		/* id = 78 */
	"MIN",		/* id = 79 */
	"MOD",		/* id = 80 */
	"MODE",		/* id = 81 */
	"MUL",		/* id = 82 */
	"NAN",		/* id = 83 */
	"NEG",		/* id = 84 */
	"NEQ",		/* id = 85 */
	"NOT",		/* id = 86 */
	"NRAND",		/* id = 87 */
	"OR",		/* id = 88 */
	"PLM",		/* id = 89 */
	"PLMg",		/* id = 90 */
	"POP",		/* id = 91 */
	"POW",		/* id = 92 */
	"PQUANT",		/* id = 93 */
	"PSI",		/* id = 94 */
	"PV",		/* id = 95 */
	"QV",		/* id = 96 */
	"R2",		/* id = 97 */
	"R2D",		/* id = 98 */
	"RAND",		/* id = 99 */
	"RINT",		/* id = 100 */
	"ROOTS",		/* id = 101 */
	"ROTT",		/* id = 102 */
	"SEC",		/* id = 103 */
	"SECD",		/* id = 104 */
	"SIGN",		/* id = 105 */
	"SIN",		/* id = 106 */
	"SINC",		/* id = 107 */
	"SIND",		/* id = 108 */
	"SINH",		/* id = 109 */
	"SKEW",		/* id = 110 */
	"SQR",		/* id = 111 */
	"SQRT",		/* id = 112 */
	"STD",		/* id = 113 */
	"STEP",		/* id = 114 */
	"STEPT",		/* id = 115 */
	"SUB",		/* id = 116 */
	"SUM",		/* id = 117 */
	"TAN",		/* id = 118 */
	"TAND",		/* id = 119 */
	"TANH",		/* id = 120 */
	"TCRIT",		/* id = 121 */
	"TDIST",		/* id = 122 */
	"TN",		/* id = 123 */
	"UPPER",		/* id = 124 */
	"XOR",		/* id = 125 */
	"Y0",		/* id = 126 */
	"Y1",		/* id = 127 */
	"YN",		/* id = 128 */
	"ZCRIT",		/* id = 129 */
	"ZDIST"		/* id = 130 */
};

