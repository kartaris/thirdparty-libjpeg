/*
 * jpegint.h
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * Modified 1997-2011 by Guido Vollbeding.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file provides common declarations for the various JPEG modules.
 * These declarations are considered internal to the JPEG library; most
 * applications using the library shouldn't need to include this file.
 */


/* Declarations for both compression & decompression */

typedef enum {			/* Operating modes for buffer controllers */
	LJPEG_JBUF_PASS_THRU,		/* Plain stripwise operation */
	/* Remaining modes require a full-image buffer to have been created */
	LJPEG_JBUF_SAVE_SOURCE,	/* Run source subobject only, save output */
	LJPEG_JBUF_CRANK_DEST,	/* Run dest subobject only, using saved data */
	LJPEG_JBUF_SAVE_AND_PASS	/* Run both subobjects, save output */
} LJPEG_J_BUF_MODE;

/* Values of global_state field (jdapi.c has some dependencies on ordering!) */
#define CSTATE_START	100	/* after create_compress */
#define CSTATE_SCANNING	101	/* start_compress done, write_scanlines OK */
#define CSTATE_RAW_OK	102	/* start_compress done, write_raw_data OK */
#define CSTATE_WRCOEFS	103	/* LJPEG_jpeg_write_coefficients done */
#define DSTATE_START	200	/* after create_decompress */
#define DSTATE_INHEADER	201	/* reading header markers, no SOS yet */
#define DSTATE_READY	202	/* found SOS, ready for start_decompress */
#define DSTATE_PRELOAD	203	/* reading multiscan file in start_decompress*/
#define DSTATE_PRESCAN	204	/* performing dummy pass for 2-pass quant */
#define DSTATE_SCANNING	205	/* start_decompress done, read_scanlines OK */
#define DSTATE_RAW_OK	206	/* start_decompress done, read_raw_data OK */
#define DSTATE_BUFIMAGE	207	/* expecting LJPEG_jpeg_start_output */
#define DSTATE_BUFPOST	208	/* looking for SOS/EOI in LJPEG_jpeg_finish_output */
#define DSTATE_RDCOEFS	209	/* reading file in LJPEG_jpeg_read_coefficients */
#define DSTATE_STOPPING	210	/* looking for EOI in LJPEG_jpeg_finish_decompress */


/* Declarations for compression modules */

/* Master control module */
struct LJPEG_jpeg_comp_master {
  LJPEG_JMETHOD(void, LJPEG_prepare_for_pass, (LJPEG_j_compress_ptr cinfo));
  LJPEG_JMETHOD(void, LJPEG_pass_startup, (LJPEG_j_compress_ptr cinfo));
  LJPEG_JMETHOD(void, LJPEG_finish_pass, (LJPEG_j_compress_ptr cinfo));

  /* State variables made visible to other modules */
  boolean call_LJPEG_pass_startup;	/* True if LJPEG_pass_startup must be called */
  boolean is_last_pass;		/* True during last pass */
};

/* Main buffer control (downsampled-data buffer) */
struct LJPEG_jpeg_c_main_controller {
  LJPEG_JMETHOD(void, LJPEG_start_pass, (LJPEG_j_compress_ptr cinfo, LJPEG_J_BUF_MODE pass_mode));
  LJPEG_JMETHOD(void, process_data, (LJPEG_j_compress_ptr cinfo,
			       LJPEG_JSAMPARRAY input_buf, LJPEG_JDIMENSION *in_row_ctr,
			       LJPEG_JDIMENSION in_rows_avail));
};

/* Compression preprocessing (downsampling input buffer control) */
struct LJPEG_jpeg_c_prep_controller {
  LJPEG_JMETHOD(void, LJPEG_start_pass, (LJPEG_j_compress_ptr cinfo, LJPEG_J_BUF_MODE pass_mode));
  LJPEG_JMETHOD(void, LJPEG_pre_process_data, (LJPEG_j_compress_ptr cinfo,
				   LJPEG_JSAMPARRAY input_buf,
				   LJPEG_JDIMENSION *in_row_ctr,
				   LJPEG_JDIMENSION in_rows_avail,
				   LJPEG_JSAMPIMAGE output_buf,
				   LJPEG_JDIMENSION *out_row_group_ctr,
				   LJPEG_JDIMENSION out_row_groups_avail));
};

/* Coefficient buffer control */
struct LJPEG_jpeg_c_coef_controller {
  LJPEG_JMETHOD(void, LJPEG_start_pass, (LJPEG_j_compress_ptr cinfo, LJPEG_J_BUF_MODE pass_mode));
  LJPEG_JMETHOD(boolean, LJPEG_compress_data, (LJPEG_j_compress_ptr cinfo,
				   LJPEG_JSAMPIMAGE input_buf));
};

/* Colorspace conversion */
struct LJPEG_jpeg_color_converter {
  LJPEG_JMETHOD(void, LJPEG_start_pass, (LJPEG_j_compress_ptr cinfo));
  LJPEG_JMETHOD(void, color_convert, (LJPEG_j_compress_ptr cinfo,
				LJPEG_JSAMPARRAY input_buf, LJPEG_JSAMPIMAGE output_buf,
				LJPEG_JDIMENSION output_row, int num_rows));
};

/* Downsampling */
struct LJPEG_jpeg_downsampler {
  LJPEG_JMETHOD(void, LJPEG_start_pass, (LJPEG_j_compress_ptr cinfo));
  LJPEG_JMETHOD(void, downsample, (LJPEG_j_compress_ptr cinfo,
			     LJPEG_JSAMPIMAGE input_buf, LJPEG_JDIMENSION in_row_index,
			     LJPEG_JSAMPIMAGE output_buf,
			     LJPEG_JDIMENSION out_row_group_index));

  boolean need_context_rows;	/* TRUE if need rows above & below */
};

/* Forward DCT (also controls coefficient quantization) */
typedef LJPEG_JMETHOD(void, LJPEG_forward_DCT_ptr,
		(LJPEG_j_compress_ptr cinfo, LJPEG_jpeg_component_info * compptr,
		 LJPEG_JSAMPARRAY sample_data, LJPEG_JBLOCKROW coef_blocks,
		 LJPEG_JDIMENSION start_row, LJPEG_JDIMENSION start_col,
		 LJPEG_JDIMENSION num_blocks));

struct LJPEG_jpeg_forward_dct {
  LJPEG_JMETHOD(void, LJPEG_start_pass, (LJPEG_j_compress_ptr cinfo));
  /* It is useful to allow each component to have a separate FDCT method. */
  LJPEG_forward_DCT_ptr LJPEG_forward_DCT[MAX_COMPONENTS];
};

/* Entropy encoding */
struct LJPEG_jpeg_entropy_encoder {
  LJPEG_JMETHOD(void, LJPEG_start_pass, (LJPEG_j_compress_ptr cinfo, boolean gather_statistics));
  LJPEG_JMETHOD(boolean, LJPEG_encode_mcu, (LJPEG_j_compress_ptr cinfo, LJPEG_JBLOCKROW *MCU_data));
  LJPEG_JMETHOD(void, LJPEG_finish_pass, (LJPEG_j_compress_ptr cinfo));
};

/* Marker writing */
struct LJPEG_jpeg_marker_writer {
  LJPEG_JMETHOD(void, LJPEG_write_file_header, (LJPEG_j_compress_ptr cinfo));
  LJPEG_JMETHOD(void, LJPEG_write_frame_header, (LJPEG_j_compress_ptr cinfo));
  LJPEG_JMETHOD(void, LJPEG_write_scan_header, (LJPEG_j_compress_ptr cinfo));
  LJPEG_JMETHOD(void, LJPEG_write_file_trailer, (LJPEG_j_compress_ptr cinfo));
  LJPEG_JMETHOD(void, LJPEG_write_tables_only, (LJPEG_j_compress_ptr cinfo));
  /* These routines are exported to allow insertion of extra markers */
  /* Probably only COM and APPn markers should be written this way */
  LJPEG_JMETHOD(void, LJPEG_write_marker_header, (LJPEG_j_compress_ptr cinfo, int marker,
				      unsigned int datalen));
  LJPEG_JMETHOD(void, write_marker_byte, (LJPEG_j_compress_ptr cinfo, int val));
};


/* Declarations for decompression modules */

/* Master control module */
struct LJPEG_jpeg_decomp_master {
  LJPEG_JMETHOD(void, LJPEG_prepare_for_output_pass, (LJPEG_j_decompress_ptr cinfo));
  LJPEG_JMETHOD(void, LJPEG_finish_output_pass, (LJPEG_j_decompress_ptr cinfo));

  /* State variables made visible to other modules */
  boolean is_dummy_pass;	/* True during 1st pass for 2-pass quant */
};

/* Input control module */
struct LJPEG_jpeg_input_controller {
  LJPEG_JMETHOD(int, consume_input, (LJPEG_j_decompress_ptr cinfo));
  LJPEG_JMETHOD(void, LJPEG_reset_input_controller, (LJPEG_j_decompress_ptr cinfo));
  LJPEG_JMETHOD(void, LJPEG_start_input_pass, (LJPEG_j_decompress_ptr cinfo));
  LJPEG_JMETHOD(void, LJPEG_finish_input_pass, (LJPEG_j_decompress_ptr cinfo));

  /* State variables made visible to other modules */
  boolean has_multiple_scans;	/* True if file has multiple scans */
  boolean eoi_reached;		/* True when EOI has been consumed */
};

/* Main buffer control (downsampled-data buffer) */
struct LJPEG_jpeg_d_main_controller {
  LJPEG_JMETHOD(void, LJPEG_start_pass, (LJPEG_j_decompress_ptr cinfo, LJPEG_J_BUF_MODE pass_mode));
  LJPEG_JMETHOD(void, process_data, (LJPEG_j_decompress_ptr cinfo,
			       LJPEG_JSAMPARRAY output_buf, LJPEG_JDIMENSION *out_row_ctr,
			       LJPEG_JDIMENSION out_rows_avail));
};

/* Coefficient buffer control */
struct LJPEG_jpeg_d_coef_controller {
  LJPEG_JMETHOD(void, LJPEG_start_input_pass, (LJPEG_j_decompress_ptr cinfo));
  LJPEG_JMETHOD(int, LJPEG_consume_data, (LJPEG_j_decompress_ptr cinfo));
  LJPEG_JMETHOD(void, LJPEG_start_output_pass, (LJPEG_j_decompress_ptr cinfo));
  LJPEG_JMETHOD(int, LJPEG_decompress_data, (LJPEG_j_decompress_ptr cinfo,
				 LJPEG_JSAMPIMAGE output_buf));
  /* Pointer to array of coefficient virtual arrays, or NULL if none */
  LJPEG_jvirt_barray_ptr *coef_arrays;
};

/* Decompression postprocessing (color quantization buffer control) */
struct LJPEG_jpeg_d_post_controller {
  LJPEG_JMETHOD(void, LJPEG_start_pass, (LJPEG_j_decompress_ptr cinfo, LJPEG_J_BUF_MODE pass_mode));
  LJPEG_JMETHOD(void, post_process_data, (LJPEG_j_decompress_ptr cinfo,
				    LJPEG_JSAMPIMAGE input_buf,
				    LJPEG_JDIMENSION *in_row_group_ctr,
				    LJPEG_JDIMENSION in_row_groups_avail,
				    LJPEG_JSAMPARRAY output_buf,
				    LJPEG_JDIMENSION *out_row_ctr,
				    LJPEG_JDIMENSION out_rows_avail));
};

/* Marker reading & parsing */
struct LJPEG_jpeg_marker_reader {
  LJPEG_JMETHOD(void, LJPEG_reset_marker_reader, (LJPEG_j_decompress_ptr cinfo));
  /* Read markers until SOS or EOI.
   * Returns same codes as are defined for LJPEG_jpeg_consume_input:
   * JPEG_SUSPENDED, JPEG_REACHED_SOS, or JPEG_REACHED_EOI.
   */
  LJPEG_JMETHOD(int, LJPEG_read_markers, (LJPEG_j_decompress_ptr cinfo));
  /* Read a restart marker --- exported for use by entropy decoder only */
  LJPEG_jpeg_marker_parser_method LJPEG_read_restart_marker;

  /* State of marker reader --- nominally internal, but applications
   * supplying COM or APPn handlers might like to know the state.
   */
  boolean saw_SOI;		/* found SOI? */
  boolean saw_SOF;		/* found SOF? */
  int next_restart_num;		/* next restart number expected (0-7) */
  unsigned int discarded_bytes;	/* # of bytes skipped looking for a marker */
};

/* Entropy decoding */
struct LJPEG_jpeg_entropy_decoder {
  LJPEG_JMETHOD(void, LJPEG_start_pass, (LJPEG_j_decompress_ptr cinfo));
  LJPEG_JMETHOD(boolean, LJPEG_decode_mcu, (LJPEG_j_decompress_ptr cinfo,
				LJPEG_JBLOCKROW *MCU_data));
};

/* Inverse DCT (also performs dequantization) */
typedef LJPEG_JMETHOD(void, inverse_DCT_method_ptr,
		(LJPEG_j_decompress_ptr cinfo, LJPEG_jpeg_component_info * compptr,
		 LJPEG_JCOEFPTR coef_block,
		 LJPEG_JSAMPARRAY output_buf, LJPEG_JDIMENSION output_col));

struct LJPEG_jpeg_inverse_dct {
  LJPEG_JMETHOD(void, LJPEG_start_pass, (LJPEG_j_decompress_ptr cinfo));
  /* It is useful to allow each component to have a separate IDCT method. */
  inverse_DCT_method_ptr inverse_DCT[MAX_COMPONENTS];
};

/* Upsampling (note that upsampler must also call color converter) */
struct LJPEG_jpeg_upsampler {
  LJPEG_JMETHOD(void, LJPEG_start_pass, (LJPEG_j_decompress_ptr cinfo));
  LJPEG_JMETHOD(void, upsample, (LJPEG_j_decompress_ptr cinfo,
			   LJPEG_JSAMPIMAGE input_buf,
			   LJPEG_JDIMENSION *in_row_group_ctr,
			   LJPEG_JDIMENSION in_row_groups_avail,
			   LJPEG_JSAMPARRAY output_buf,
			   LJPEG_JDIMENSION *out_row_ctr,
			   LJPEG_JDIMENSION out_rows_avail));

  boolean need_context_rows;	/* TRUE if need rows above & below */
};

/* Colorspace conversion */
struct LJPEG_jpeg_color_deconverter {
  LJPEG_JMETHOD(void, LJPEG_start_pass, (LJPEG_j_decompress_ptr cinfo));
  LJPEG_JMETHOD(void, color_convert, (LJPEG_j_decompress_ptr cinfo,
				LJPEG_JSAMPIMAGE input_buf, LJPEG_JDIMENSION input_row,
				LJPEG_JSAMPARRAY output_buf, int num_rows));
};

/* Color quantization or color precision reduction */
struct LJPEG_jpeg_color_quantizer {
  LJPEG_JMETHOD(void, LJPEG_start_pass, (LJPEG_j_decompress_ptr cinfo, boolean is_pre_scan));
  LJPEG_JMETHOD(void, LJPEG_color_quantize, (LJPEG_j_decompress_ptr cinfo,
				 LJPEG_JSAMPARRAY input_buf, LJPEG_JSAMPARRAY output_buf,
				 int num_rows));
  LJPEG_JMETHOD(void, LJPEG_finish_pass, (LJPEG_j_decompress_ptr cinfo));
  LJPEG_JMETHOD(void, new_color_map, (LJPEG_j_decompress_ptr cinfo));
};


/* Miscellaneous useful macros */

#undef MAX
#define MAX(a,b)	((a) > (b) ? (a) : (b))
#undef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))


/* We assume that right shift corresponds to signed division by 2 with
 * rounding towards minus infinity.  This is correct for typical "arithmetic
 * shift" instructions that shift in copies of the sign bit.  But some
 * C compilers implement >> with an unsigned shift.  For these machines you
 * must define RIGHT_SHIFT_IS_UNSIGNED.
 * RIGHT_SHIFT provides a proper signed right shift of an INT32 quantity.
 * It is only applied with constant shift counts.  SHIFT_TEMPS must be
 * included in the variables of any routine using RIGHT_SHIFT.
 */

#ifdef RIGHT_SHIFT_IS_UNSIGNED
#define SHIFT_TEMPS	INT32 shift_temp;
#define RIGHT_SHIFT(x,shft)  \
	((shift_temp = (x)) < 0 ? \
	 (shift_temp >> (shft)) | ((~((INT32) 0)) << (32-(shft))) : \
	 (shift_temp >> (shft)))
#else
#define SHIFT_TEMPS
#define RIGHT_SHIFT(x,shft)	((x) >> (shft))
#endif


/* Short forms of external names for systems with brain-damaged linkers. */

#ifdef NEED_SHORT_EXTERNAL_NAMES
#define LJPEG_jinit_compress_master	jICompress
#define LJPEG_jinit_c_master_control	jICMaster
#define LJPEG_jinit_c_main_controller	jICMainC
#define LJPEG_jinit_c_prep_controller	jICPrepC
#define LJPEG_jinit_c_coef_controller	jICCoefC
#define LJPEG_jinit_color_converter	jICColor
#define LJPEG_jinit_downsampler	jIDownsampler
#define LJPEG_jinit_forward_dct	jIFDCT
#define LJPEG_jinit_huff_encoder	jIHEncoder
#define LJPEG_jinit_LJPEG_arith_encoder	jIAEncoder
#define LJPEG_jinit_marker_writer	jIMWriter
#define LJPEG_jinit_master_decompress	jIDMaster
#define LJPEG_jinit_d_main_controller	jIDMainC
#define LJPEG_jinit_d_coef_controller	jIDCoefC
#define LJPEG_jinit_d_post_controller	jIDPostC
#define LJPEG_jinit_input_controller	jIInCtlr
#define LJPEG_jinit_marker_reader	jIMReader
#define LJPEG_jinit_huff_decoder	jIHDecoder
#define LJPEG_jinit_arith_decoder	jIADecoder
#define LJPEG_jinit_inverse_dct	jIIDCT
#define LJPEG_jinit_upsampler		jIUpsampler
#define LJPEG_jinit_color_deconverter	jIDColor
#define LJPEG_jinit_1pass_quantizer	jI1Quant
#define LJPEG_jinit_2pass_quantizer	jI2Quant
#define LJPEG_jinit_merged_upsampler	jIMUpsampler
#define LJPEG_jinit_memory_mgr	jIMemMgr
#define LJPEG_jdiv_round_up		jDivRound
#define LJPEG_jround_up		jRound
#define LJPEG_jzero_far		jZeroFar
#define LJPEG_jcopy_sample_rows	jCopySamples
#define LJPEG_jcopy_block_row		jCopyBlocks
#define LJPEG_jpeg_zigzag_order	jZIGTable
#define LJPEG_jpeg_natural_order	jZAGTable
#define LJPEG_jpeg_natural_order7	jZAG7Table
#define LJPEG_jpeg_natural_order6	jZAG6Table
#define LJPEG_jpeg_natural_order5	jZAG5Table
#define LJPEG_jpeg_natural_order4	jZAG4Table
#define LJPEG_jpeg_natural_order3	jZAG3Table
#define LJPEG_jpeg_natural_order2	jZAG2Table
#define LJPEG_jpeg_aritab		jAriTab
#endif /* NEED_SHORT_EXTERNAL_NAMES */


/* On normal machines we can apply MEMCOPY() and MEMZERO() to sample arrays
 * and coefficient-block arrays.  This won't work on 80x86 because the arrays
 * are FAR and we're assuming a small-pointer memory model.  However, some
 * DOS compilers provide far-pointer versions of memcpy() and memset() even
 * in the small-model libraries.  These will be used if USE_FMEM is defined.
 * Otherwise, the routines in jutils.c do it the hard way.
 */

#ifndef NEED_FAR_POINTERS	/* normal case, same as regular macro */
#define FMEMZERO(target,size)	MEMZERO(target,size)
#else				/* 80x86 case */
#ifdef USE_FMEM
#define FMEMZERO(target,size)	_fmemset((void FAR *)(target), 0, (size_t)(size))
#else
EXTERN(void) LJPEG_jzero_far LJPEG_JPP((void FAR * target, size_t bytestozero));
#define FMEMZERO(target,size)	LJPEG_jzero_far(target, size)
#endif
#endif


/* Compression module initialization routines */
EXTERN(void) LJPEG_jinit_compress_master LJPEG_JPP((LJPEG_j_compress_ptr cinfo));
EXTERN(void) LJPEG_jinit_c_master_control LJPEG_JPP((LJPEG_j_compress_ptr cinfo,
					 boolean transcode_only));
EXTERN(void) LJPEG_jinit_c_main_controller LJPEG_JPP((LJPEG_j_compress_ptr cinfo,
					  boolean need_full_buffer));
EXTERN(void) LJPEG_jinit_c_prep_controller LJPEG_JPP((LJPEG_j_compress_ptr cinfo,
					  boolean need_full_buffer));
EXTERN(void) LJPEG_jinit_c_coef_controller LJPEG_JPP((LJPEG_j_compress_ptr cinfo,
					  boolean need_full_buffer));
EXTERN(void) LJPEG_jinit_color_converter LJPEG_JPP((LJPEG_j_compress_ptr cinfo));
EXTERN(void) LJPEG_jinit_downsampler LJPEG_JPP((LJPEG_j_compress_ptr cinfo));
EXTERN(void) LJPEG_jinit_forward_dct LJPEG_JPP((LJPEG_j_compress_ptr cinfo));
EXTERN(void) LJPEG_jinit_huff_encoder LJPEG_JPP((LJPEG_j_compress_ptr cinfo));
EXTERN(void) LJPEG_jinit_LJPEG_arith_encoder LJPEG_JPP((LJPEG_j_compress_ptr cinfo));
EXTERN(void) LJPEG_jinit_marker_writer LJPEG_JPP((LJPEG_j_compress_ptr cinfo));
/* Decompression module initialization routines */
EXTERN(void) LJPEG_jinit_master_decompress LJPEG_JPP((LJPEG_j_decompress_ptr cinfo));
EXTERN(void) LJPEG_jinit_d_main_controller LJPEG_JPP((LJPEG_j_decompress_ptr cinfo,
					  boolean need_full_buffer));
EXTERN(void) LJPEG_jinit_d_coef_controller LJPEG_JPP((LJPEG_j_decompress_ptr cinfo,
					  boolean need_full_buffer));
EXTERN(void) LJPEG_jinit_d_post_controller LJPEG_JPP((LJPEG_j_decompress_ptr cinfo,
					  boolean need_full_buffer));
EXTERN(void) LJPEG_jinit_input_controller LJPEG_JPP((LJPEG_j_decompress_ptr cinfo));
EXTERN(void) LJPEG_jinit_marker_reader LJPEG_JPP((LJPEG_j_decompress_ptr cinfo));
EXTERN(void) LJPEG_jinit_huff_decoder LJPEG_JPP((LJPEG_j_decompress_ptr cinfo));
EXTERN(void) LJPEG_jinit_arith_decoder LJPEG_JPP((LJPEG_j_decompress_ptr cinfo));
EXTERN(void) LJPEG_jinit_inverse_dct LJPEG_JPP((LJPEG_j_decompress_ptr cinfo));
EXTERN(void) LJPEG_jinit_upsampler LJPEG_JPP((LJPEG_j_decompress_ptr cinfo));
EXTERN(void) LJPEG_jinit_color_deconverter LJPEG_JPP((LJPEG_j_decompress_ptr cinfo));
EXTERN(void) LJPEG_jinit_1pass_quantizer LJPEG_JPP((LJPEG_j_decompress_ptr cinfo));
EXTERN(void) LJPEG_jinit_2pass_quantizer LJPEG_JPP((LJPEG_j_decompress_ptr cinfo));
EXTERN(void) LJPEG_jinit_merged_upsampler LJPEG_JPP((LJPEG_j_decompress_ptr cinfo));
/* Memory manager initialization */
EXTERN(void) LJPEG_jinit_memory_mgr LJPEG_JPP((LJPEG_j_common_ptr cinfo));

/* Utility routines in jutils.c */
EXTERN(long) LJPEG_jdiv_round_up LJPEG_JPP((long a, long b));
EXTERN(long) LJPEG_jround_up LJPEG_JPP((long a, long b));
EXTERN(void) LJPEG_jcopy_sample_rows LJPEG_JPP((LJPEG_JSAMPARRAY input_array, int source_row,
				    LJPEG_JSAMPARRAY output_array, int dest_row,
				    int num_rows, LJPEG_JDIMENSION num_cols));
EXTERN(void) LJPEG_jcopy_block_row LJPEG_JPP((LJPEG_JBLOCKROW input_row, LJPEG_JBLOCKROW output_row,
				  LJPEG_JDIMENSION num_blocks));
/* Constant tables in jutils.c */
#if 0				/* This table is not actually needed in v6a */
extern const int LJPEG_jpeg_zigzag_order[]; /* natural coef order to zigzag order */
#endif
extern const int LJPEG_jpeg_natural_order[]; /* zigzag coef order to natural order */
extern const int LJPEG_jpeg_natural_order7[]; /* zz to natural order for 7x7 block */
extern const int LJPEG_jpeg_natural_order6[]; /* zz to natural order for 6x6 block */
extern const int LJPEG_jpeg_natural_order5[]; /* zz to natural order for 5x5 block */
extern const int LJPEG_jpeg_natural_order4[]; /* zz to natural order for 4x4 block */
extern const int LJPEG_jpeg_natural_order3[]; /* zz to natural order for 3x3 block */
extern const int LJPEG_jpeg_natural_order2[]; /* zz to natural order for 2x2 block */

/* Arithmetic coding probability estimation tables in jaricom.c */
extern const INT32 LJPEG_jpeg_aritab[];

/* Suppress undefined-structure complaints if necessary. */

#ifdef INCOMPLETE_TYPES_BROKEN
#ifndef AM_MEMORY_MANAGER	/* only jmemmgr.c defines these */
struct LJPEG_jvirt_sarray_control { long dummy; };
struct LJPEG_jvirt_barray_control { long dummy; };
#endif
#endif /* INCOMPLETE_TYPES_BROKEN */
