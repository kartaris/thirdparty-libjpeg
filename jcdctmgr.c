/*
 * jcdctmgr.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains the forward-DCT management logic.
 * This code selects a particular DCT implementation to be used,
 * and it performs related housekeeping chores including coefficient
 * quantization.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"		/* Private declarations for DCT subsystem */


/* Private subobject for this module */

typedef struct {
  struct LJPEG_jpeg_forward_dct pub;	/* public fields */

  /* Pointer to the DCT routine actually in use */
  LJPEG_forward_DCT_method_ptr do_dct[MAX_COMPONENTS];

  /* The actual post-DCT divisors --- not identical to the quant table
   * entries, because of scaling (especially for an unnormalized DCT).
   * Each table is given in normal array order.
   */
  DCTELEM * divisors[NUM_QUANT_TBLS];

#ifdef DCT_FLOAT_SUPPORTED
  /* Same as above for the floating-point case. */
  float_DCT_method_ptr do_float_dct[MAX_COMPONENTS];
  FAST_FLOAT * float_divisors[NUM_QUANT_TBLS];
#endif
} LJPEG_my_fdct_controller;

typedef LJPEG_my_fdct_controller * LJPEG_my_fdct_ptr;


/* The current scaled-DCT routines require ISLOW-style divisor tables,
 * so be sure to compile that code if either ISLOW or SCALING is requested.
 */
#ifdef DCT_ISLOW_SUPPORTED
#define PROVIDE_ISLOW_TABLES
#else
#ifdef DCT_SCALING_SUPPORTED
#define PROVIDE_ISLOW_TABLES
#endif
#endif


/*
 * Perform forward DCT on one or more blocks of a component.
 *
 * The input samples are taken from the sample_data[] array starting at
 * position start_row/start_col, and moving to the right for any additional
 * blocks. The quantized coefficients are returned in coef_blocks[].
 */

LJPEG_METHODDEF(void)
LJPEG_forward_DCT (LJPEG_j_compress_ptr cinfo, LJPEG_jpeg_component_info * compptr,
	     LJPEG_JSAMPARRAY sample_data, LJPEG_JBLOCKROW coef_blocks,
	     LJPEG_JDIMENSION start_row, LJPEG_JDIMENSION start_col,
	     LJPEG_JDIMENSION num_blocks)
/* This version is used for integer DCT implementations. */
{
  /* This routine is heavily used, so it's worth coding it tightly. */
  LJPEG_my_fdct_ptr fdct = (LJPEG_my_fdct_ptr) cinfo->fdct;
  LJPEG_forward_DCT_method_ptr do_dct = fdct->do_dct[compptr->component_index];
  DCTELEM * divisors = fdct->divisors[compptr->quant_tbl_no];
  DCTELEM workspace[DCTSIZE2];	/* work area for FDCT subroutine */
  LJPEG_JDIMENSION bi;

  sample_data += start_row;	/* fold in the vertical offset once */

  for (bi = 0; bi < num_blocks; bi++, start_col += compptr->DCT_h_scaled_size) {
    /* Perform the DCT */
    (*do_dct) (workspace, sample_data, start_col);

    /* Quantize/descale the coefficients, and store into coef_blocks[] */
    { register DCTELEM temp, qval;
      register int i;
      register LJPEG_JCOEFPTR output_ptr = coef_blocks[bi];

      for (i = 0; i < DCTSIZE2; i++) {
	qval = divisors[i];
	temp = workspace[i];
	/* Divide the coefficient value by qval, ensuring proper rounding.
	 * Since C does not specify the direction of rounding for negative
	 * quotients, we have to force the dividend positive for portability.
	 *
	 * In most files, at least half of the output values will be zero
	 * (at default quantization settings, more like three-quarters...)
	 * so we should ensure that this case is fast.  On many machines,
	 * a comparison is enough cheaper than a divide to make a special test
	 * a win.  Since both inputs will be nonnegative, we need only test
	 * for a < b to discover whether a/b is 0.
	 * If your machine's division is fast enough, define FAST_DIVIDE.
	 */
#ifdef FAST_DIVIDE
#define DIVIDE_BY(a,b)	a /= b
#else
#define DIVIDE_BY(a,b)	if (a >= b) a /= b; else a = 0
#endif
	if (temp < 0) {
	  temp = -temp;
	  temp += qval>>1;	/* for rounding */
	  DIVIDE_BY(temp, qval);
	  temp = -temp;
	} else {
	  temp += qval>>1;	/* for rounding */
	  DIVIDE_BY(temp, qval);
	}
	output_ptr[i] = (JCOEF) temp;
      }
    }
  }
}


#ifdef DCT_FLOAT_SUPPORTED

LJPEG_METHODDEF(void)
LJPEG_forward_DCT_float (LJPEG_j_compress_ptr cinfo, LJPEG_jpeg_component_info * compptr,
		   LJPEG_JSAMPARRAY sample_data, LJPEG_JBLOCKROW coef_blocks,
		   LJPEG_JDIMENSION start_row, LJPEG_JDIMENSION start_col,
		   LJPEG_JDIMENSION num_blocks)
/* This version is used for floating-point DCT implementations. */
{
  /* This routine is heavily used, so it's worth coding it tightly. */
  LJPEG_my_fdct_ptr fdct = (LJPEG_my_fdct_ptr) cinfo->fdct;
  float_DCT_method_ptr do_dct = fdct->do_float_dct[compptr->component_index];
  FAST_FLOAT * divisors = fdct->float_divisors[compptr->quant_tbl_no];
  FAST_FLOAT workspace[DCTSIZE2]; /* work area for FDCT subroutine */
  LJPEG_JDIMENSION bi;

  sample_data += start_row;	/* fold in the vertical offset once */

  for (bi = 0; bi < num_blocks; bi++, start_col += compptr->DCT_h_scaled_size) {
    /* Perform the DCT */
    (*do_dct) (workspace, sample_data, start_col);

    /* Quantize/descale the coefficients, and store into coef_blocks[] */
    { register FAST_FLOAT temp;
      register int i;
      register LJPEG_JCOEFPTR output_ptr = coef_blocks[bi];

      for (i = 0; i < DCTSIZE2; i++) {
	/* Apply the quantization and scaling factor */
	temp = workspace[i] * divisors[i];
	/* Round to nearest integer.
	 * Since C does not specify the direction of rounding for negative
	 * quotients, we have to force the dividend positive for portability.
	 * The maximum coefficient size is +-16K (for 12-bit data), so this
	 * code should work for either 16-bit or 32-bit ints.
	 */
	output_ptr[i] = (JCOEF) ((int) (temp + (FAST_FLOAT) 16384.5) - 16384);
      }
    }
  }
}

#endif /* DCT_FLOAT_SUPPORTED */


/*
 * Initialize for a processing pass.
 * Verify that all referenced Q-tables are present, and set up
 * the divisor table for each one.
 * In the current implementation, DCT of all components is done during
 * the first pass, even if only some components will be output in the
 * first scan.  Hence all components should be examined here.
 */

LJPEG_METHODDEF(void)
LJPEG_start_pass_fdctmgr (LJPEG_j_compress_ptr cinfo)
{
  LJPEG_my_fdct_ptr fdct = (LJPEG_my_fdct_ptr) cinfo->fdct;
  int ci, qtblno, i;
  LJPEG_jpeg_component_info *compptr;
  int method = 0;
  LJPEG_JQUANT_TBL * qtbl;
  DCTELEM * dtbl;

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    /* Select the proper DCT routine for this component's scaling */
    switch ((compptr->DCT_h_scaled_size << 8) + compptr->DCT_v_scaled_size) {
#ifdef DCT_SCALING_SUPPORTED
    case ((1 << 8) + 1):
      fdct->do_dct[ci] = jpeg_fdct_1x1;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((2 << 8) + 2):
      fdct->do_dct[ci] = jpeg_fdct_2x2;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((3 << 8) + 3):
      fdct->do_dct[ci] = jpeg_fdct_3x3;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((4 << 8) + 4):
      fdct->do_dct[ci] = jpeg_fdct_4x4;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((5 << 8) + 5):
      fdct->do_dct[ci] = jpeg_fdct_5x5;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((6 << 8) + 6):
      fdct->do_dct[ci] = jpeg_fdct_6x6;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((7 << 8) + 7):
      fdct->do_dct[ci] = jpeg_fdct_7x7;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((9 << 8) + 9):
      fdct->do_dct[ci] = jpeg_fdct_9x9;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((10 << 8) + 10):
      fdct->do_dct[ci] = jpeg_fdct_10x10;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((11 << 8) + 11):
      fdct->do_dct[ci] = jpeg_fdct_11x11;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((12 << 8) + 12):
      fdct->do_dct[ci] = jpeg_fdct_12x12;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((13 << 8) + 13):
      fdct->do_dct[ci] = jpeg_fdct_13x13;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((14 << 8) + 14):
      fdct->do_dct[ci] = jpeg_fdct_14x14;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((15 << 8) + 15):
      fdct->do_dct[ci] = jpeg_fdct_15x15;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((16 << 8) + 16):
      fdct->do_dct[ci] = jpeg_fdct_16x16;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((16 << 8) + 8):
      fdct->do_dct[ci] = jpeg_fdct_16x8;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((14 << 8) + 7):
      fdct->do_dct[ci] = jpeg_fdct_14x7;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((12 << 8) + 6):
      fdct->do_dct[ci] = jpeg_fdct_12x6;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((10 << 8) + 5):
      fdct->do_dct[ci] = jpeg_fdct_10x5;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((8 << 8) + 4):
      fdct->do_dct[ci] = jpeg_fdct_8x4;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((6 << 8) + 3):
      fdct->do_dct[ci] = jpeg_fdct_6x3;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((4 << 8) + 2):
      fdct->do_dct[ci] = jpeg_fdct_4x2;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((2 << 8) + 1):
      fdct->do_dct[ci] = jpeg_fdct_2x1;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((8 << 8) + 16):
      fdct->do_dct[ci] = jpeg_fdct_8x16;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((7 << 8) + 14):
      fdct->do_dct[ci] = jpeg_fdct_7x14;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((6 << 8) + 12):
      fdct->do_dct[ci] = jpeg_fdct_6x12;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((5 << 8) + 10):
      fdct->do_dct[ci] = jpeg_fdct_5x10;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((4 << 8) + 8):
      fdct->do_dct[ci] = jpeg_fdct_4x8;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((3 << 8) + 6):
      fdct->do_dct[ci] = jpeg_fdct_3x6;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((2 << 8) + 4):
      fdct->do_dct[ci] = jpeg_fdct_2x4;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
    case ((1 << 8) + 2):
      fdct->do_dct[ci] = jpeg_fdct_1x2;
      method = JDCT_ISLOW;	/* jfdctint uses islow-style table */
      break;
#endif
    case ((DCTSIZE << 8) + DCTSIZE):
      switch (cinfo->dct_method) {
#ifdef DCT_ISLOW_SUPPORTED
      case JDCT_ISLOW:
	fdct->do_dct[ci] = jpeg_fdct_islow;
	method = JDCT_ISLOW;
	break;
#endif
#ifdef DCT_IFAST_SUPPORTED
      case JDCT_IFAST:
	fdct->do_dct[ci] = jpeg_fdct_ifast;
	method = JDCT_IFAST;
	break;
#endif
#ifdef DCT_FLOAT_SUPPORTED
      case JDCT_FLOAT:
	fdct->do_float_dct[ci] = jpeg_fdct_float;
	method = JDCT_FLOAT;
	break;
#endif
      default:
	ERREXIT(cinfo, JERR_NOT_COMPILED);
	break;
      }
      break;
    default:
      ERREXIT2(cinfo, JERR_BAD_DCTSIZE,
	       compptr->DCT_h_scaled_size, compptr->DCT_v_scaled_size);
      break;
    }
    qtblno = compptr->quant_tbl_no;
    /* Make sure specified quantization table is present */
    if (qtblno < 0 || qtblno >= NUM_QUANT_TBLS ||
	cinfo->quant_tbl_ptrs[qtblno] == NULL)
      ERREXIT1(cinfo, JERR_NO_QUANT_TABLE, qtblno);
    qtbl = cinfo->quant_tbl_ptrs[qtblno];
    /* Compute divisors for this quant table */
    /* We may do this more than once for same table, but it's not a big deal */
    switch (method) {
#ifdef PROVIDE_ISLOW_TABLES
    case JDCT_ISLOW:
      /* For LL&M IDCT method, divisors are equal to raw quantization
       * coefficients multiplied by 8 (to counteract scaling).
       */
      if (fdct->divisors[qtblno] == NULL) {
	fdct->divisors[qtblno] = (DCTELEM *)
	  (*cinfo->mem->alloc_small) ((LJPEG_j_common_ptr) cinfo, JPOOL_IMAGE,
				      DCTSIZE2 * SIZEOF(DCTELEM));
      }
      dtbl = fdct->divisors[qtblno];
      for (i = 0; i < DCTSIZE2; i++) {
	dtbl[i] = ((DCTELEM) qtbl->quantval[i]) << 3;
      }
      fdct->pub.LJPEG_forward_DCT[ci] = LJPEG_forward_DCT;
      break;
#endif
#ifdef DCT_IFAST_SUPPORTED
    case JDCT_IFAST:
      {
	/* For AA&N IDCT method, divisors are equal to quantization
	 * coefficients scaled by scalefactor[row]*scalefactor[col], where
	 *   scalefactor[0] = 1
	 *   scalefactor[k] = cos(k*PI/16) * sqrt(2)    for k=1..7
	 * We apply a further scale factor of 8.
	 */
#define CONST_BITS 14
	static const INT16 aanscales[DCTSIZE2] = {
	  /* precomputed values scaled up by 14 bits */
	  16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
	  22725, 31521, 29692, 26722, 22725, 17855, 12299,  6270,
	  21407, 29692, 27969, 25172, 21407, 16819, 11585,  5906,
	  19266, 26722, 25172, 22654, 19266, 15137, 10426,  5315,
	  16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
	  12873, 17855, 16819, 15137, 12873, 10114,  6967,  3552,
	   8867, 12299, 11585, 10426,  8867,  6967,  4799,  2446,
	   4520,  6270,  5906,  5315,  4520,  3552,  2446,  1247
	};
	SHIFT_TEMPS

	if (fdct->divisors[qtblno] == NULL) {
	  fdct->divisors[qtblno] = (DCTELEM *)
	    (*cinfo->mem->alloc_small) ((LJPEG_j_common_ptr) cinfo, JPOOL_IMAGE,
					DCTSIZE2 * SIZEOF(DCTELEM));
	}
	dtbl = fdct->divisors[qtblno];
	for (i = 0; i < DCTSIZE2; i++) {
	  dtbl[i] = (DCTELEM)
	    DESCALE(MULTIPLY16V16((INT32) qtbl->quantval[i],
				  (INT32) aanscales[i]),
		    CONST_BITS-3);
	}
      }
      fdct->pub.LJPEG_forward_DCT[ci] = LJPEG_forward_DCT;
      break;
#endif
#ifdef DCT_FLOAT_SUPPORTED
    case JDCT_FLOAT:
      {
	/* For float AA&N IDCT method, divisors are equal to quantization
	 * coefficients scaled by scalefactor[row]*scalefactor[col], where
	 *   scalefactor[0] = 1
	 *   scalefactor[k] = cos(k*PI/16) * sqrt(2)    for k=1..7
	 * We apply a further scale factor of 8.
	 * What's actually stored is 1/divisor so that the inner loop can
	 * use a multiplication rather than a division.
	 */
	FAST_FLOAT * fdtbl;
	int row, col;
	static const double aanscalefactor[DCTSIZE] = {
	  1.0, 1.387039845, 1.306562965, 1.175875602,
	  1.0, 0.785694958, 0.541196100, 0.275899379
	};

	if (fdct->float_divisors[qtblno] == NULL) {
	  fdct->float_divisors[qtblno] = (FAST_FLOAT *)
	    (*cinfo->mem->alloc_small) ((LJPEG_j_common_ptr) cinfo, JPOOL_IMAGE,
					DCTSIZE2 * SIZEOF(FAST_FLOAT));
	}
	fdtbl = fdct->float_divisors[qtblno];
	i = 0;
	for (row = 0; row < DCTSIZE; row++) {
	  for (col = 0; col < DCTSIZE; col++) {
	    fdtbl[i] = (FAST_FLOAT)
	      (1.0 / (((double) qtbl->quantval[i] *
		       aanscalefactor[row] * aanscalefactor[col] * 8.0)));
	    i++;
	  }
	}
      }
      fdct->pub.LJPEG_forward_DCT[ci] = LJPEG_forward_DCT_float;
      break;
#endif
    default:
      ERREXIT(cinfo, JERR_NOT_COMPILED);
      break;
    }
  }
}


/*
 * Initialize FDCT manager.
 */

LJPEG_GLOBAL(void)
LJPEG_jinit_forward_dct (LJPEG_j_compress_ptr cinfo)
{
  LJPEG_my_fdct_ptr fdct;
  int i;

  fdct = (LJPEG_my_fdct_ptr)
    (*cinfo->mem->alloc_small) ((LJPEG_j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(LJPEG_my_fdct_controller));
  cinfo->fdct = (struct LJPEG_jpeg_forward_dct *) fdct;
  fdct->pub.LJPEG_start_pass = LJPEG_start_pass_fdctmgr;

  /* Mark divisor tables unallocated */
  for (i = 0; i < NUM_QUANT_TBLS; i++) {
    fdct->divisors[i] = NULL;
#ifdef DCT_FLOAT_SUPPORTED
    fdct->float_divisors[i] = NULL;
#endif
  }
}
