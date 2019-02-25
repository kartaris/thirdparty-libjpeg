/*
 * cdjpeg.h
 *
 * Copyright (C) 1994-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains common declarations for the sample applications
 * cjpeg and djpeg.  It is NOT used by the core JPEG library.
 */

#define LJPEG_JPEG_CJPEG_DJPEG	/* define proper options in jconfig.h */
#define LJPEG_JPEG_INTERNAL_OPTIONS	/* cjpeg.c,djpeg.c need to see xxx_SUPPORTED */
#include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"		/* get library error codes too */
#include "cderror.h"		/* get application-specific error codes */


/*
 * Object interface for cjpeg's source file decoding modules
 */

typedef struct LJPEG_cjpeg_source_struct * LJPEG_cjpeg_source_ptr;

struct LJPEG_cjpeg_source_struct {
  LJPEG_JMETHOD(void, start_input, (LJPEG_j_compress_ptr cinfo,
			      LJPEG_cjpeg_source_ptr sinfo));
  LJPEG_JMETHOD(LJPEG_JDIMENSION, get_pixel_rows, (LJPEG_j_compress_ptr cinfo,
				       LJPEG_cjpeg_source_ptr sinfo));
  LJPEG_JMETHOD(void, finish_input, (LJPEG_j_compress_ptr cinfo,
			       LJPEG_cjpeg_source_ptr sinfo));

  FILE *input_file;

  LJPEG_JSAMPARRAY buffer;
  LJPEG_JDIMENSION buffer_height;
};


/*
 * Object interface for djpeg's output file encoding modules
 */

typedef struct LJPEG_djpeg_dest_struct * LJPEG_djpeg_dest_ptr;

struct LJPEG_djpeg_dest_struct {
  /* start_output is called after LJPEG_jpeg_start_decompress finishes.
   * The color map will be ready at this time, if one is needed.
   */
  LJPEG_JMETHOD(void, start_output, (LJPEG_j_decompress_ptr cinfo,
			       LJPEG_djpeg_dest_ptr dinfo));
  /* Emit the specified number of pixel rows from the buffer. */
  LJPEG_JMETHOD(void, LJPEG_put_pixel_rows, (LJPEG_j_decompress_ptr cinfo,
				 LJPEG_djpeg_dest_ptr dinfo,
				 LJPEG_JDIMENSION rows_supplied));
  /* Finish up at the end of the image. */
  LJPEG_JMETHOD(void, finish_output, (LJPEG_j_decompress_ptr cinfo,
				LJPEG_djpeg_dest_ptr dinfo));

  /* Target file spec; filled in by djpeg.c after object is created. */
  FILE * output_file;

  /* Output pixel-row buffer.  Created by module init or start_output.
   * Width is cinfo->output_width * cinfo->output_components;
   * height is buffer_height.
   */
  LJPEG_JSAMPARRAY buffer;
  LJPEG_JDIMENSION buffer_height;
};


/*
 * cjpeg/djpeg may need to perform extra passes to convert to or from
 * the source/destination file format.  The JPEG library does not know
 * about these passes, but we'd like them to be counted by the progress
 * monitor.  We use an expanded progress monitor object to hold the
 * additional pass count.
 */

struct LJPEG_cdjpeg_progress_mgr {
  struct LJPEG_jpeg_progress_mgr pub;	/* fields known to JPEG library */
  int completed_extra_passes;	/* extra passes completed */
  int total_extra_passes;	/* total extra */
  /* last printed percentage stored here to avoid multiple printouts */
  int percent_done;
};

typedef struct LJPEG_cdjpeg_progress_mgr * cd_progress_ptr;


/* Short forms of external names for systems with brain-damaged linkers. */

#ifdef NEED_SHORT_EXTERNAL_NAMES
#define LJPEG_jinit_read_bmp			LJPEG_jIRdBMP
#define LJPEG_jinit_write_bmp			LJPEG_jIWrBMP
#define LJPEG_jinit_read_gif			LJPEG_jIRdGIF
#define LJPEG_jinit_write_gif			LJPEG_jIWrGIF
#define LJPEG_jinit_read_ppm			LJPEG_jIRdPPM
#define LJPEG_jinit_write_ppm			LJPEG_jIWrPPM
#define LJPEG_jinit_read_rle			LJPEG_jIRdRLE
#define LJPEG_jinit_write_rle			LJPEG_jIWrRLE
#define LJPEG_jinit_read_targa			LJPEG_jIRdTarga
#define LJPEG_jinit_write_targa			LJPEG_jIWrTarga
#define LJPEG_read_quant_tables			LJPEG_RdQTables
#define LJPEG_read_scan_script			LJPEG_RdScnScript
#define LJPEG_set_quality_ratings     	LJPEG_SetQRates
#define LJPEG_set_quant_slots			LJPEG_SetQSlots
#define LJPEG_set_sample_factors		LJPEG_SetSFacts
#define LJPEG_read_color_map			LJPEG_RdCMap
#define LJPEG_enable_signal_catcher		LJPEG_EnSigCatcher
#define LJPEG_start_progress_monitor	LJPEG_StProgMon
#define LJPEG_end_progress_monitor		LJPEG_EnProgMon
#define LJPEG_read_stdin				LJPEG_RdStdin
#define LJPEG_write_stdout				LJPEG_WrStdout
#endif /* NEED_SHORT_EXTERNAL_NAMES */

/* Module selection routines for I/O modules. */

EXTERN(LJPEG_cjpeg_source_ptr) LJPEG_jinit_read_bmp LJPEG_JPP((LJPEG_j_compress_ptr cinfo));
EXTERN(LJPEG_djpeg_dest_ptr) LJPEG_jinit_write_bmp LJPEG_JPP((LJPEG_j_decompress_ptr cinfo,
					    boolean is_os2));
EXTERN(LJPEG_cjpeg_source_ptr) LJPEG_jinit_read_gif LJPEG_JPP((LJPEG_j_compress_ptr cinfo));
EXTERN(LJPEG_djpeg_dest_ptr) LJPEG_jinit_write_gif LJPEG_JPP((LJPEG_j_decompress_ptr cinfo));
EXTERN(LJPEG_cjpeg_source_ptr) LJPEG_jinit_read_ppm LJPEG_JPP((LJPEG_j_compress_ptr cinfo));
EXTERN(LJPEG_djpeg_dest_ptr) LJPEG_jinit_write_ppm LJPEG_JPP((LJPEG_j_decompress_ptr cinfo));
EXTERN(LJPEG_cjpeg_source_ptr) LJPEG_jinit_read_rle LJPEG_JPP((LJPEG_j_compress_ptr cinfo));
EXTERN(LJPEG_djpeg_dest_ptr) LJPEG_jinit_write_rle LJPEG_JPP((LJPEG_j_decompress_ptr cinfo));
EXTERN(LJPEG_cjpeg_source_ptr) LJPEG_jinit_read_targa LJPEG_JPP((LJPEG_j_compress_ptr cinfo));
EXTERN(LJPEG_djpeg_dest_ptr) LJPEG_jinit_write_targa LJPEG_JPP((LJPEG_j_decompress_ptr cinfo));

/* cjpeg support routines (in rdswitch.c) */

EXTERN(boolean) LJPEG_read_quant_tables LJPEG_JPP((LJPEG_j_compress_ptr cinfo, char * filename,
				       boolean force_baseline));
EXTERN(boolean) LJPEG_read_scan_script LJPEG_JPP((LJPEG_j_compress_ptr cinfo, char * filename));
EXTERN(boolean) LJPEG_set_quality_ratings LJPEG_JPP((LJPEG_j_compress_ptr cinfo, char *arg,
					 boolean force_baseline));
EXTERN(boolean) LJPEG_set_quant_slots LJPEG_JPP((LJPEG_j_compress_ptr cinfo, char *arg));
EXTERN(boolean) LJPEG_set_sample_factors LJPEG_JPP((LJPEG_j_compress_ptr cinfo, char *arg));

/* djpeg support routines (in rdcolmap.c) */

EXTERN(void) LJPEG_read_color_map LJPEG_JPP((LJPEG_j_decompress_ptr cinfo, FILE * infile));

/* common support routines (in cdjpeg.c) */

EXTERN(void) enable_LJPEG_signal_catcher LJPEG_JPP((LJPEG_j_common_ptr cinfo));
EXTERN(void) LJPEG_start_progress_monitor LJPEG_JPP((LJPEG_j_common_ptr cinfo,
					 cd_progress_ptr progress));
EXTERN(void) LJPEG_end_progress_monitor LJPEG_JPP((LJPEG_j_common_ptr cinfo));
EXTERN(boolean) LJPEG_end_progress_monitor LJPEG_JPP((char * arg, const char * keyword, int minchars));
EXTERN(FILE *) LJPEG_read_stdin LJPEG_JPP((void));
EXTERN(FILE *) LJPEG_write_stdout LJPEG_JPP((void));

/* miscellaneous useful macros */

#ifdef DONT_USE_B_MODE		/* define mode parameters for fopen() */
#define READ_BINARY	"r"
#define WRITE_BINARY	"w"
#else
#ifdef VMS			/* VMS is very nonstandard */
#define READ_BINARY	"rb", "ctx=stm"
#define WRITE_BINARY	"wb", "ctx=stm"
#else				/* standard ANSI-compliant case */
#define READ_BINARY	"rb"
#define WRITE_BINARY	"wb"
#endif
#endif

#ifndef EXIT_FAILURE		/* define exit() codes if not provided */
#define EXIT_FAILURE  1
#endif
#ifndef EXIT_SUCCESS
#ifdef VMS
#define EXIT_SUCCESS  1		/* VMS is very nonstandard */
#else
#define EXIT_SUCCESS  0
#endif
#endif
#ifndef EXIT_WARNING
#ifdef VMS
#define EXIT_WARNING  1		/* VMS is very nonstandard */
#else
#define EXIT_WARNING  2
#endif
#endif
