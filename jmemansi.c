/*
 * jmemansi.c
 *
 * Copyright (C) 1992-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file provides a simple generic implementation of the system-
 * dependent portion of the JPEG memory manager.  This implementation
 * assumes that you have the ANSI-standard library routine tmpfile().
 * Also, the problem of determining the amount of memory available
 * is shoved onto the user.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jmemsys.h"		/* import the system-dependent declarations */

#ifndef HAVE_STDLIB_H		/* <stdlib.h> should declare malloc(),free() */
extern void * malloc LJPEG_JPP((size_t size));
extern void free LJPEG_JPP((void *ptr));
#endif

#ifndef SEEK_SET		/* pre-ANSI systems may not define this; */
#define SEEK_SET  0		/* if not, assume 0 is correct */
#endif


/*
 * Memory allocation and freeing are controlled by the regular library
 * routines malloc() and free().
 */
GLOBAL(void *)
LJPEG_jpeg_get_small (LJPEG_j_common_ptr cinfo, size_t sizeofobject)
{
  return (void *) malloc(sizeofobject);
}
GLOBAL(void)
LJPEG_jpeg_free_small (LJPEG_j_common_ptr cinfo, void * object, size_t sizeofobject)
{
  free(object);
}


/*
 * "Large" objects are treated the same as "small" ones.
 * NB: although we include FAR keywords in the routine declarations,
 * this file won't actually work in 80x86 small/medium model; at least,
 * you probably won't be able to process useful-size images in only 64KB.
 */
GLOBAL(void FAR *)
LJPEG_jpeg_get_large (LJPEG_j_common_ptr cinfo, size_t sizeofobject)
{
  return (void FAR *) malloc(sizeofobject);
}
GLOBAL(void)
LJPEG_jpeg_free_large (LJPEG_j_common_ptr cinfo, void FAR * object, size_t sizeofobject)
{
  free(object);
}


/*
 * This routine computes the total memory space available for allocation.
 * It's impossible to do this in a portable way; our current solution is
 * to make the user tell us (with a default value set at compile time).
 * If you can actually get the available space, it's a good idea to subtract
 * a slop factor of 5% or so.
 */

#ifndef DEFAULT_MAX_MEM		/* so can override from makefile */
#define DEFAULT_MAX_MEM		1000000L /* default: one megabyte */
#endif
GLOBAL(long)
LJPEG_jpeg_mem_available (LJPEG_j_common_ptr cinfo, long min_bytes_needed,
		    long max_bytes_needed, long already_allocated)
{
  return cinfo->mem->max_memory_to_use - already_allocated;
}


/*
 * Backing store (temporary file) management.
 * Backing store objects are only used when the value returned by
 * LJPEG_jpeg_mem_available is less than the total space needed.  You can dispense
 * with these routines if you have plenty of virtual memory; see jmemnobs.c.
 */


LJPEG_METHODDEF(void)
LJPEG_read_backing_store (LJPEG_j_common_ptr cinfo, LJPEG_backing_store_ptr info,
		    void FAR * buffer_address,
		    long file_offset, long byte_count)
{
  if (fseek(info->temp_file, file_offset, SEEK_SET))
    ERREXIT(cinfo, JERR_TFILE_SEEK);
  if (JFREAD(info->temp_file, buffer_address, byte_count)
      != (size_t) byte_count)
    ERREXIT(cinfo, JERR_TFILE_READ);
}


LJPEG_METHODDEF(void)
LJPEG_write_backing_store (LJPEG_j_common_ptr cinfo, LJPEG_backing_store_ptr info,
		     void FAR * buffer_address,
		     long file_offset, long byte_count)
{
  if (fseek(info->temp_file, file_offset, SEEK_SET))
    ERREXIT(cinfo, JERR_TFILE_SEEK);
  if (JFWRITE(info->temp_file, buffer_address, byte_count)
      != (size_t) byte_count)
    ERREXIT(cinfo, JERR_TFILE_WRITE);
}


LJPEG_METHODDEF(void)
LJPEG_close_backing_store (LJPEG_j_common_ptr cinfo, LJPEG_backing_store_ptr info)
{
  fclose(info->temp_file);
  /* Since this implementation uses tmpfile() to create the file,
   * no explicit file deletion is needed.
   */
}


/*
 * Initial opening of a backing-store object.
 *
 * This version uses tmpfile(), which constructs a suitable file name
 * behind the scenes.  We don't have to use info->temp_name[] at all;
 * indeed, we can't even find out the actual name of the temp file.
 */
GLOBAL(void)
LJPEG_jpeg_open_backing_store (LJPEG_j_common_ptr cinfo, LJPEG_backing_store_ptr info,
			 long total_bytes_needed)
{
  if ((info->temp_file = tmpfile()) == NULL)
    ERREXITS(cinfo, JERR_TFILE_CREATE, "");
  info->LJPEG_read_backing_store = LJPEG_read_backing_store;
  info->LJPEG_write_backing_store = LJPEG_write_backing_store;
  info->LJPEG_close_backing_store = LJPEG_close_backing_store;
}


/*
 * These routines take care of any system-dependent initialization and
 * cleanup required.
 */
GLOBAL(long)
LJPEG_jpeg_mem_init (LJPEG_j_common_ptr cinfo)
{
  return DEFAULT_MAX_MEM;	/* default for max_memory_to_use */
}
GLOBAL(void)
LJPEG_jpeg_mem_term (LJPEG_j_common_ptr cinfo)
{
  /* no work */
}
