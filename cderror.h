/*
 * cderror.h
 *
 * Copyright (C) 1994-1997, Thomas G. Lane.
 * Modified 2009 by Guido Vollbeding.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file defines the error and message codes for the cjpeg/djpeg
 * applications.  These strings are not needed as part of the JPEG library
 * proper.
 * Edit this file to add new codes, or to translate the message strings to
 * some other language.
 */

/*
 * To define the enum list of message codes, include this file without
 * defining macro LJPEG_JMESSAGE.  To create a message string table, include it
 * again with a suitable LJPEG_JMESSAGE definition (see jerror.c for an example).
 */
#ifndef LJPEG_JMESSAGE
#ifndef LJPEG_CDERROR_H
#define LJPEG_CDERROR_H
/* First time through, define the enum list */
#define LJPEG_JMAKE_ENUM_LIST
#else
/* Repeated inclusions of this file are no-ops unless LJPEG_JMESSAGE is defined */
#define LJPEG_JMESSAGE(code,string)
#endif /* LJPEG_CDERROR_H */
#endif /* LJPEG_JMESSAGE */

#ifdef LJPEG_JMAKE_ENUM_LIST

typedef enum {

#define LJPEG_JMESSAGE(code,string)	code ,

#endif /* LJPEG_JMAKE_ENUM_LIST */

LJPEG_JMESSAGE(JMSG_FIRSTADDONCODE=1000, NULL) /* Must be first entry! */

#ifdef LJPEG_BMP_SUPPORTED
LJPEG_JMESSAGE(JERR_BMP_BADCMAP, "Unsupported BMP colormap format")
LJPEG_JMESSAGE(JERR_BMP_BADDEPTH, "Only 8- and 24-bit BMP files are supported")
LJPEG_JMESSAGE(JERR_BMP_BADHEADER, "Invalid BMP file: bad header length")
LJPEG_JMESSAGE(JERR_BMP_BADPLANES, "Invalid BMP file: biPlanes not equal to 1")
LJPEG_JMESSAGE(JERR_BMP_COLORSPACE, "BMP output must be grayscale or RGB")
LJPEG_JMESSAGE(JERR_BMP_COMPRESSED, "Sorry, compressed BMPs not yet supported")
LJPEG_JMESSAGE(JERR_BMP_EMPTY, "Empty BMP image")
LJPEG_JMESSAGE(JERR_BMP_NOT, "Not a BMP file - does not start with BM")
LJPEG_JMESSAGE(JTRC_BMP, "%ux%u 24-bit BMP image")
LJPEG_JMESSAGE(JTRC_BMP_MAPPED, "%ux%u 8-bit colormapped BMP image")
LJPEG_JMESSAGE(JTRC_BMP_OS2, "%ux%u 24-bit OS2 BMP image")
LJPEG_JMESSAGE(JTRC_BMP_OS2_MAPPED, "%ux%u 8-bit colormapped OS2 BMP image")
#endif /* LJPEG_BMP_SUPPORTED */

#ifdef LJPEG_GIF_SUPPORTED
LJPEG_JMESSAGE(JERR_GIF_BUG, "GIF output got confused")
LJPEG_JMESSAGE(JERR_GIF_CODESIZE, "Bogus GIF codesize %d")
LJPEG_JMESSAGE(JERR_GIF_COLORSPACE, "GIF output must be grayscale or RGB")
LJPEG_JMESSAGE(JERR_GIF_IMAGENOTFOUND, "Too few images in GIF file")
LJPEG_JMESSAGE(JERR_GIF_NOT, "Not a GIF file")
LJPEG_JMESSAGE(JTRC_GIF, "%ux%ux%d GIF image")
LJPEG_JMESSAGE(JTRC_GIF_BADVERSION,
	 "Warning: unexpected GIF version number '%c%c%c'")
LJPEG_JMESSAGE(JTRC_GIF_EXTENSION, "Ignoring GIF extension block of type 0x%02x")
LJPEG_JMESSAGE(JTRC_GIF_NONSQUARE, "Caution: nonsquare pixels in input")
LJPEG_JMESSAGE(JWRN_GIF_BADDATA, "Corrupt data in GIF file")
LJPEG_JMESSAGE(JWRN_GIF_CHAR, "Bogus char 0x%02x in GIF file, ignoring")
LJPEG_JMESSAGE(JWRN_GIF_ENDCODE, "Premature end of GIF image")
LJPEG_JMESSAGE(JWRN_GIF_NOMOREDATA, "Ran out of GIF bits")
#endif /* LJPEG_GIF_SUPPORTED */

#ifdef LJPEG_PPM_SUPPORTED
LJPEG_JMESSAGE(JERR_PPM_COLORSPACE, "PPM output must be grayscale or RGB")
LJPEG_JMESSAGE(JERR_PPM_NONNUMERIC, "Nonnumeric data in PPM file")
LJPEG_JMESSAGE(JERR_PPM_NOT, "Not a PPM/PGM file")
LJPEG_JMESSAGE(JTRC_PGM, "%ux%u PGM image")
LJPEG_JMESSAGE(JTRC_PGM_TEXT, "%ux%u text PGM image")
LJPEG_JMESSAGE(JTRC_PPM, "%ux%u PPM image")
LJPEG_JMESSAGE(JTRC_PPM_TEXT, "%ux%u text PPM image")
#endif /* LJPEG_PPM_SUPPORTED */

#ifdef LJPEG_RLE_SUPPORTED
LJPEG_JMESSAGE(JERR_RLE_BADERROR, "Bogus error code from RLE library")
LJPEG_JMESSAGE(JERR_RLE_COLORSPACE, "RLE output must be grayscale or RGB")
LJPEG_JMESSAGE(JERR_RLE_DIMENSIONS, "Image dimensions (%ux%u) too large for RLE")
LJPEG_JMESSAGE(JERR_RLE_EMPTY, "Empty RLE file")
LJPEG_JMESSAGE(JERR_RLE_EOF, "Premature EOF in RLE header")
LJPEG_JMESSAGE(JERR_RLE_MEM, "Insufficient memory for RLE header")
LJPEG_JMESSAGE(JERR_RLE_NOT, "Not an RLE file")
LJPEG_JMESSAGE(JERR_RLE_TOOMANYCHANNELS, "Cannot handle %d output channels for RLE")
LJPEG_JMESSAGE(JERR_RLE_UNSUPPORTED, "Cannot handle this RLE setup")
LJPEG_JMESSAGE(JTRC_RLE, "%ux%u full-color RLE file")
LJPEG_JMESSAGE(JTRC_RLE_FULLMAP, "%ux%u full-color RLE file with map of length %d")
LJPEG_JMESSAGE(JTRC_RLE_GRAY, "%ux%u grayscale RLE file")
LJPEG_JMESSAGE(JTRC_RLE_MAPGRAY, "%ux%u grayscale RLE file with map of length %d")
LJPEG_JMESSAGE(JTRC_RLE_MAPPED, "%ux%u colormapped RLE file with map of length %d")
#endif /* LJPEG_RLE_SUPPORTED */

#ifdef LJPEG_TARGA_SUPPORTED
LJPEG_JMESSAGE(JERR_TGA_BADCMAP, "Unsupported Targa colormap format")
LJPEG_JMESSAGE(JERR_TGA_BADPARMS, "Invalid or unsupported Targa file")
LJPEG_JMESSAGE(JERR_TGA_COLORSPACE, "Targa output must be grayscale or RGB")
LJPEG_JMESSAGE(JTRC_TGA, "%ux%u RGB Targa image")
LJPEG_JMESSAGE(JTRC_TGA_GRAY, "%ux%u grayscale Targa image")
LJPEG_JMESSAGE(JTRC_TGA_MAPPED, "%ux%u colormapped Targa image")
#else
LJPEG_JMESSAGE(JERR_TGA_NOTCOMP, "Targa support was not compiled")
#endif /* LJPEG_TARGA_SUPPORTED */

LJPEG_JMESSAGE(JERR_BAD_CMAP_FILE,
	 "Color map file is invalid or of unsupported format")
LJPEG_JMESSAGE(JERR_TOO_MANY_COLORS,
	 "Output file format cannot handle %d colormap entries")
LJPEG_JMESSAGE(JERR_UNGETC_FAILED, "ungetc failed")
#ifdef LJPEG_TARGA_SUPPORTED
LJPEG_JMESSAGE(JERR_UNKNOWN_FORMAT,
	 "Unrecognized input file format --- perhaps you need -targa")
#else
LJPEG_JMESSAGE(JERR_UNKNOWN_FORMAT, "Unrecognized input file format")
#endif
LJPEG_JMESSAGE(JERR_UNSUPPORTED_FORMAT, "Unsupported output file format")

#ifdef LJPEG_JMAKE_ENUM_LIST

  JMSG_LASTADDONCODE
} LJPEG_ADDON_MESSAGE_CODE;

#undef LJPEG_JMAKE_ENUM_LIST
#endif /* LJPEG_JMAKE_ENUM_LIST */

/* Zap LJPEG_JMESSAGE macro so that future re-inclusions do nothing by default */
#undef LJPEG_JMESSAGE
