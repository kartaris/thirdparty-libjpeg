/*
 * transupp.h
 *
 * Copyright (C) 1997-2011, Thomas G. Lane, Guido Vollbeding.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains declarations for image transformation routines and
 * other utility code used by the jpegtran sample application.  These are
 * NOT part of the core JPEG library.  But we keep these routines separate
 * from jpegtran.c to ease the task of maintaining jpegtran-like programs
 * that have other user interfaces.
 *
 * NOTE: all the routines declared here have very specific requirements
 * about when they are to be executed during the reading and writing of the
 * source and destination files.  See the comments in transupp.c, or see
 * jpegtran.c for an example of correct usage.
 */

/* If you happen not to want the image transform support, disable it here */
#ifndef TRANSFORMS_SUPPORTED
#define TRANSFORMS_SUPPORTED 1		/* 0 disables transform code */
#endif

/*
 * Although rotating and flipping data expressed as DCT coefficients is not
 * hard, there is an asymmetry in the JPEG format specification for images
 * whose dimensions aren't multiples of the iMCU size.  The right and bottom
 * image edges are padded out to the next iMCU boundary with junk data; but
 * no padding is possible at the top and left edges.  If we were to flip
 * the whole image including the pad data, then pad garbage would become
 * visible at the top and/or left, and real pixels would disappear into the
 * pad margins --- perhaps permanently, since encoders & decoders may not
 * bother to preserve DCT blocks that appear to be completely outside the
 * nominal image area.  So, we have to exclude any partial iMCUs from the
 * basic transformation.
 *
 * Transpose is the only transformation that can handle partial iMCUs at the
 * right and bottom edges completely cleanly.  flip_h can flip partial iMCUs
 * at the bottom, but leaves any partial iMCUs at the right edge untouched.
 * Similarly flip_v leaves any partial iMCUs at the bottom edge untouched.
 * The other transforms are defined as combinations of these basic transforms
 * and process edge blocks in a way that preserves the equivalence.
 *
 * The "trim" option causes untransformable partial iMCUs to be dropped;
 * this is not strictly lossless, but it usually gives the best-looking
 * result for odd-size images.  Note that when this option is active,
 * the expected mathematical equivalences between the transforms may not hold.
 * (For example, -rot 270 -trim trims only the bottom edge, but -rot 90 -trim
 * followed by -rot 180 -trim trims both edges.)
 *
 * We also offer a lossless-crop option, which discards data outside a given
 * image region but losslessly preserves what is inside.  Like the rotate and
 * flip transforms, lossless crop is restricted by the JPEG format: the upper
 * left corner of the selected region must fall on an iMCU boundary.  If this
 * does not hold for the given crop parameters, we silently move the upper left
 * corner up and/or left to make it so, simultaneously increasing the region
 * dimensions to keep the lower right crop corner unchanged.  (Thus, the
 * output image covers at least the requested region, but may cover more.)
 * The adjustment of the region dimensions may be optionally disabled.
 *
 * We also provide a lossless-resize option, which is kind of a lossless-crop
 * operation in the DCT coefficient block domain - it discards higher-order
 * coefficients and losslessly preserves lower-order coefficients of a
 * sub-block.
 *
 * Rotate/flip transform, resize, and crop can be requested together in a
 * single invocation.  The crop is applied last --- that is, the crop region
 * is specified in terms of the destination image after transform/resize.
 *
 * We also offer a "force to grayscale" option, which simply discards the
 * chrominance channels of a YCbCr image.  This is lossless in the sense that
 * the luminance channel is preserved exactly.  It's not the same kind of
 * thing as the rotate/flip transformations, but it's convenient to handle it
 * as part of this package, mainly because the transformation routines have to
 * be aware of the option to know how many components to work on.
 */


/* Short forms of external names for systems with brain-damaged linkers. */

#ifdef NEED_SHORT_EXTERNAL_NAMES
#define LJPEG_jtransform_parse_crop_spec	LJPEG_jTrParCrop
#define LJPEG_jtransform_request_workspace	LJPEG_jTrRequest
#define LJPEG_jtransform_adjust_parameters	LJPEG_jTrAdjust
#define LJPEG_jtransform_execute_transform	LJPEG_jTrExec
#define LJPEG_jtransform_perfect_transform	LJPEG_jTrPerfect
#define LJPEG_jcopy_markers_setup			LJPEG_jCMrkSetup
#define LJPEG_jcopy_markers_execute			LJPEG_jCMrkExec
#endif /* NEED_SHORT_EXTERNAL_NAMES */


/*
 * Codes for supported types of image transformations.
 */

typedef enum {
	JXFORM_NONE,		/* no transformation */
	JXFORM_FLIP_H,		/* horizontal flip */
	JXFORM_FLIP_V,		/* vertical flip */
	JXFORM_TRANSPOSE,	/* transpose across UL-to-LR axis */
	JXFORM_TRANSVERSE,	/* transpose across UR-to-LL axis */
	JXFORM_ROT_90,		/* 90-degree clockwise rotation */
	JXFORM_ROT_180,		/* 180-degree rotation */
	JXFORM_ROT_270		/* 270-degree clockwise (or 90 ccw) */
} LJPEG_JXFORM_CODE;

/*
 * Codes for crop parameters, which can individually be unspecified,
 * positive or negative for xoffset or yoffset,
 * positive or forced for width or height.
 */

typedef enum {
        JCROP_UNSET,
        JCROP_POS,
        JCROP_NEG,
        JCROP_FORCE
} LJPEG_JCROP_CODE;

/*
 * Transform parameters struct.
 * NB: application must not change any elements of this struct after
 * calling jtransform_request_workspace.
 */

typedef struct {
  /* Options: set by caller */
  LJPEG_JXFORM_CODE transform;	/* image transform operator */
  boolean perfect;		/* if TRUE, fail if partial MCUs are requested */
  boolean trim;			/* if TRUE, trim partial MCUs as needed */
  boolean force_grayscale;	/* if TRUE, convert color image to grayscale */
  boolean crop;			/* if TRUE, crop source image */

  /* Crop parameters: application need not set these unless crop is TRUE.
   * These can be filled in by jtransform_parse_crop_spec().
   */
  LJPEG_JDIMENSION crop_width;	/* Width of selected region */
  LJPEG_JCROP_CODE crop_width_set;	/* (forced disables adjustment) */
  LJPEG_JDIMENSION crop_height;	/* Height of selected region */
  LJPEG_JCROP_CODE crop_height_set;	/* (forced disables adjustment) */
  LJPEG_JDIMENSION crop_xoffset;	/* X offset of selected region */
  LJPEG_JCROP_CODE crop_xoffset_set;	/* (negative measures from right edge) */
  LJPEG_JDIMENSION crop_yoffset;	/* Y offset of selected region */
  LJPEG_JCROP_CODE crop_yoffset_set;	/* (negative measures from bottom edge) */

  /* Internal workspace: caller should not touch these */
  int num_components;		/* # of components in workspace */
  LJPEG_jvirt_barray_ptr * workspace_coef_arrays; /* workspace for transformations */
  LJPEG_JDIMENSION output_width;	/* cropped destination dimensions */
  LJPEG_JDIMENSION output_height;
  LJPEG_JDIMENSION x_crop_offset;	/* destination crop offsets measured in iMCUs */
  LJPEG_JDIMENSION y_crop_offset;
  int iMCU_sample_width;	/* destination iMCU size */
  int iMCU_sample_height;
} LJPEG_jpeg_transform_info;


#if TRANSFORMS_SUPPORTED

/* Parse a crop specification (written in X11 geometry style) */
EXTERN(boolean) LJPEG_jtransform_parse_crop_spec
	LJPEG_JPP((LJPEG_jpeg_transform_info *info, const char *spec));
/* Request any required workspace */
EXTERN(boolean) LJPEG_jtransform_request_workspace
	LJPEG_JPP((LJPEG_j_decompress_ptr srcinfo, LJPEG_jpeg_transform_info *info));
/* Adjust output image parameters */
EXTERN(LJPEG_jvirt_barray_ptr *) LJPEG_jtransform_adjust_parameters
	LJPEG_JPP((LJPEG_j_decompress_ptr srcinfo, LJPEG_j_compress_ptr dstinfo,
	     LJPEG_jvirt_barray_ptr *src_coef_arrays,
	     LJPEG_jpeg_transform_info *info));
/* Execute the actual transformation, if any */
EXTERN(void) LJPEG_jtransform_execute_transform
	LJPEG_JPP((LJPEG_j_decompress_ptr srcinfo, LJPEG_j_compress_ptr dstinfo,
	     LJPEG_jvirt_barray_ptr *src_coef_arrays,
	     LJPEG_jpeg_transform_info *info));
/* Determine whether lossless transformation is perfectly
 * possible for a specified image and transformation.
 */
EXTERN(boolean) LJPEG_jtransform_perfect_transform
	LJPEG_JPP((LJPEG_JDIMENSION image_width, LJPEG_JDIMENSION image_height,
	     int MCU_width, int MCU_height,
	     LJPEG_JXFORM_CODE transform));

/* jtransform_execute_transform used to be called
 * jtransform_execute_transformation, but some compilers complain about
 * routine names that long.  This macro is here to avoid breaking any
 * old source code that uses the original name...
 */
#define LJPEG_jtransform_execute_transformation	LJPEG_jtransform_execute_transform

#endif /* TRANSFORMS_SUPPORTED */


/*
 * Support for copying optional markers from source to destination file.
 */

typedef enum {
	JCOPYOPT_NONE,		/* copy no optional markers */
	JCOPYOPT_COMMENTS,	/* copy only comment (COM) markers */
	JCOPYOPT_ALL		/* copy all optional markers */
} LJPEG_JCOPY_OPTION;

#define JCOPYOPT_DEFAULT  JCOPYOPT_COMMENTS	/* recommended default */

/* Setup decompression object to save desired markers in memory */
EXTERN(void) LJPEG_jcopy_markers_setup
	LJPEG_JPP((LJPEG_j_decompress_ptr srcinfo, LJPEG_JCOPY_OPTION option));
/* Copy markers saved in the given source object to the destination object */
EXTERN(void) LJPEG_jcopy_markers_execute
	LJPEG_JPP((LJPEG_j_decompress_ptr srcinfo, LJPEG_j_compress_ptr dstinfo,
	     LJPEG_JCOPY_OPTION option));
