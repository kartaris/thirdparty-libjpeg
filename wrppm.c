/*
 * wrppm.c
 *
 * Copyright (C) 1991-1996, Thomas G. Lane.
 * Modified 2009 by Guido Vollbeding.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to write output images in PPM/PGM format.
 * The extended 2-byte-per-sample raw PPM/PGM formats are supported.
 * The PBMPLUS library is NOT required to compile this software
 * (but it is highly useful as a set of PPM image manipulation programs).
 *
 * These routines may need modification for non-Unix environments or
 * specialized applications.  As they stand, they assume output to
 * an ordinary stdio stream.
 */

#include "cdjpeg.h"		/* Common decls for cjpeg/djpeg applications */

#ifdef LJPEG_PPM_SUPPORTED


/*
 * For 12-bit JPEG data, we either downscale the values to 8 bits
 * (to write standard byte-per-sample PPM/PGM files), or output
 * nonstandard word-per-sample PPM/PGM files.  Downscaling is done
 * if PPM_NORAWWORD is defined (this can be done in the Makefile
 * or in jconfig.h).
 * (When the core library supports data precision reduction, a cleaner
 * implementation will be to ask for that instead.)
 */

#if BITS_IN_JSAMPLE == 8
#define PUTPPMSAMPLE(ptr,v)  *ptr++ = (char) (v)
#define BYTESPERSAMPLE 1
#define PPM_MAXVAL 255
#else
#ifdef PPM_NORAWWORD
#define PUTPPMSAMPLE(ptr,v)  *ptr++ = (char) ((v) >> (BITS_IN_JSAMPLE-8))
#define BYTESPERSAMPLE 1
#define PPM_MAXVAL 255
#else
/* The word-per-sample format always puts the MSB first. */
#define PUTPPMSAMPLE(ptr,v)			\
	{ register int val_ = v;		\
	  *ptr++ = (char) ((val_ >> 8) & 0xFF);	\
	  *ptr++ = (char) (val_ & 0xFF);	\
	}
#define BYTESPERSAMPLE 2
#define PPM_MAXVAL ((1<<BITS_IN_JSAMPLE)-1)
#endif
#endif


/*
 * When LJPEG_JSAMPLE is the same size as char, we can just fwrite() the
 * decompressed data to the PPM or PGM file.  On PCs, in order to make this
 * work the output buffer must be allocated in near data space, because we are
 * assuming small-data memory model wherein fwrite() can't reach far memory.
 * If you need to process very wide images on a PC, you might have to compile
 * in large-memory model, or else replace fwrite() with a putc() loop ---
 * which will be much slower.
 */


/* Private version of data destination object */

typedef struct {
  struct LJPEG_djpeg_dest_struct pub;	/* public fields */

  /* Usually these two pointers point to the same place: */
  char *iobuffer;		/* fwrite's I/O buffer */
  LJPEG_JSAMPROW pixrow;		/* decompressor output buffer */
  size_t buffer_width;		/* width of I/O buffer */
  LJPEG_JDIMENSION samples_per_row;	/* JSAMPLEs per output row */
} LJPEG_ppm_dest_struct;

typedef LJPEG_ppm_dest_struct * LJPEG_ppm_dest_ptr;


/*
 * Write some pixel data.
 * In this module rows_supplied will always be 1.
 *
 * put_pixel_rows handles the "normal" 8-bit case where the decompressor
 * output buffer is physically the same as the fwrite buffer.
 */

LJPEG_METHODDEF(void)
LJPEG_put_pixel_rows (LJPEG_j_decompress_ptr cinfo, LJPEG_djpeg_dest_ptr dinfo,
		LJPEG_JDIMENSION rows_supplied)
{
  LJPEG_ppm_dest_ptr dest = (LJPEG_ppm_dest_ptr) dinfo;

  (void) JFWRITE(dest->pub.output_file, dest->iobuffer, dest->buffer_width);
}


/*
 * This code is used when we have to copy the data and apply a pixel
 * format translation.  Typically this only happens in 12-bit mode.
 */

LJPEG_METHODDEF(void)
LJPEG_copy_pixel_rows (LJPEG_j_decompress_ptr cinfo, LJPEG_djpeg_dest_ptr dinfo,
		 LJPEG_JDIMENSION rows_supplied)
{
  LJPEG_ppm_dest_ptr dest = (LJPEG_ppm_dest_ptr) dinfo;
  register char * bufferptr;
  register LJPEG_JSAMPROW ptr;
  register LJPEG_JDIMENSION col;

  ptr = dest->pub.buffer[0];
  bufferptr = dest->iobuffer;
  for (col = dest->samples_per_row; col > 0; col--) {
    PUTPPMSAMPLE(bufferptr, GETJSAMPLE(*ptr++));
  }
  (void) JFWRITE(dest->pub.output_file, dest->iobuffer, dest->buffer_width);
}


/*
 * Write some pixel data when color quantization is in effect.
 * We have to demap the color index values to straight data.
 */

LJPEG_METHODDEF(void)
LJPEG_put_demapped_rgb (LJPEG_j_decompress_ptr cinfo, LJPEG_djpeg_dest_ptr dinfo,
		  LJPEG_JDIMENSION rows_supplied)
{
  LJPEG_ppm_dest_ptr dest = (LJPEG_ppm_dest_ptr) dinfo;
  register char * bufferptr;
  register int pixval;
  register LJPEG_JSAMPROW ptr;
  register LJPEG_JSAMPROW color_map0 = cinfo->colormap[0];
  register LJPEG_JSAMPROW color_map1 = cinfo->colormap[1];
  register LJPEG_JSAMPROW color_map2 = cinfo->colormap[2];
  register LJPEG_JDIMENSION col;

  ptr = dest->pub.buffer[0];
  bufferptr = dest->iobuffer;
  for (col = cinfo->output_width; col > 0; col--) {
    pixval = GETJSAMPLE(*ptr++);
    PUTPPMSAMPLE(bufferptr, GETJSAMPLE(color_map0[pixval]));
    PUTPPMSAMPLE(bufferptr, GETJSAMPLE(color_map1[pixval]));
    PUTPPMSAMPLE(bufferptr, GETJSAMPLE(color_map2[pixval]));
  }
  (void) JFWRITE(dest->pub.output_file, dest->iobuffer, dest->buffer_width);
}


LJPEG_METHODDEF(void)
LJPEG_put_demapped_gray (LJPEG_j_decompress_ptr cinfo, LJPEG_djpeg_dest_ptr dinfo,
		   LJPEG_JDIMENSION rows_supplied)
{
  LJPEG_ppm_dest_ptr dest = (LJPEG_ppm_dest_ptr) dinfo;
  register char * bufferptr;
  register LJPEG_JSAMPROW ptr;
  register LJPEG_JSAMPROW color_map = cinfo->colormap[0];
  register LJPEG_JDIMENSION col;

  ptr = dest->pub.buffer[0];
  bufferptr = dest->iobuffer;
  for (col = cinfo->output_width; col > 0; col--) {
    PUTPPMSAMPLE(bufferptr, GETJSAMPLE(color_map[GETJSAMPLE(*ptr++)]));
  }
  (void) JFWRITE(dest->pub.output_file, dest->iobuffer, dest->buffer_width);
}


/*
 * Startup: write the file header.
 */

LJPEG_METHODDEF(void)
LJPEG_start_output_ppm (LJPEG_j_decompress_ptr cinfo, LJPEG_djpeg_dest_ptr dinfo)
{
  LJPEG_ppm_dest_ptr dest = (LJPEG_ppm_dest_ptr) dinfo;

  /* Emit file header */
  switch (cinfo->out_color_space) {
  case LJPEG_JCS_GRAYSCALE:
    /* emit header for raw PGM format */
    fprintf(dest->pub.output_file, "P5\n%ld %ld\n%d\n",
	    (long) cinfo->output_width, (long) cinfo->output_height,
	    PPM_MAXVAL);
    break;
  case LJPEG_JCS_RGB:
    /* emit header for raw PPM format */
    fprintf(dest->pub.output_file, "P6\n%ld %ld\n%d\n",
	    (long) cinfo->output_width, (long) cinfo->output_height,
	    PPM_MAXVAL);
    break;
  default:
    ERREXIT(cinfo, JERR_PPM_COLORSPACE);
  }
}


/*
 * Finish up at the end of the file.
 */

LJPEG_METHODDEF(void)
LJPEG_finish_output_ppm (LJPEG_j_decompress_ptr cinfo, LJPEG_djpeg_dest_ptr dinfo)
{
  /* Make sure we wrote the output file OK */
  fflush(dinfo->output_file);
  if (ferror(dinfo->output_file))
    ERREXIT(cinfo, JERR_FILE_WRITE);
}


/*
 * The module selection routine for PPM format output.
 */

LJPEG_GLOBAL(LJPEG_djpeg_dest_ptr)
LJPEG_jinit_write_ppm (LJPEG_j_decompress_ptr cinfo)
{
  LJPEG_ppm_dest_ptr dest;

  /* Create module interface object, fill in method pointers */
  dest = (LJPEG_ppm_dest_ptr)
      (*cinfo->mem->LJPEG_alloc_small) ((LJPEG_j_common_ptr) cinfo, JPOOL_IMAGE,
				  SIZEOF(LJPEG_ppm_dest_struct));
  dest->pub.start_output = LJPEG_start_output_ppm;
  dest->pub.finish_output = LJPEG_finish_output_ppm;

  /* Calculate output image dimensions so we can allocate space */
  LJPEG_jpeg_calc_output_dimensions(cinfo);

  /* Create physical I/O buffer.  Note we make this near on a PC. */
  dest->samples_per_row = cinfo->output_width * cinfo->out_color_components;
  dest->buffer_width = dest->samples_per_row * (BYTESPERSAMPLE * SIZEOF(char));
  dest->iobuffer = (char *) (*cinfo->mem->LJPEG_alloc_small)
    ((LJPEG_j_common_ptr) cinfo, JPOOL_IMAGE, dest->buffer_width);

  if (cinfo->quantize_colors || BITS_IN_JSAMPLE != 8 ||
      SIZEOF(LJPEG_JSAMPLE) != SIZEOF(char)) {
    /* When quantizing, we need an output buffer for colormap indexes
     * that's separate from the physical I/O buffer.  We also need a
     * separate buffer if pixel format translation must take place.
     */
    dest->pub.buffer = (*cinfo->mem->LJPEG_alloc_sarray)
      ((LJPEG_j_common_ptr) cinfo, JPOOL_IMAGE,
       cinfo->output_width * cinfo->output_components, (LJPEG_JDIMENSION) 1);
    dest->pub.buffer_height = 1;
    if (! cinfo->quantize_colors)
      dest->pub.LJPEG_put_pixel_rows = LJPEG_copy_pixel_rows;
    else if (cinfo->out_color_space == LJPEG_JCS_GRAYSCALE)
      dest->pub.LJPEG_put_pixel_rows = LJPEG_put_demapped_gray;
    else
      dest->pub.LJPEG_put_pixel_rows = LJPEG_put_demapped_rgb;
  } else {
    /* We will fwrite() directly from decompressor output buffer. */
    /* Synthesize a LJPEG_JSAMPARRAY pointer structure */
    /* Cast here implies near->far pointer conversion on PCs */
    dest->pixrow = (LJPEG_JSAMPROW) dest->iobuffer;
    dest->pub.buffer = & dest->pixrow;
    dest->pub.buffer_height = 1;
    dest->pub.LJPEG_put_pixel_rows = LJPEG_put_pixel_rows;
  }

  return (LJPEG_djpeg_dest_ptr) dest;
}

#endif /* LJPEG_PPM_SUPPORTED */
