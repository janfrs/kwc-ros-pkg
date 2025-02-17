//
// C version of stereo algorithm
// Basic column-oriented library
//
// Copyright 2008 by Kurt Konolige
// Videre Design
// kurt@videredesign.com
//

// This file is copied over from stereo/ost. All the functions are
// therefore being put into a namespace of ost_
// We may want to consolidate the multiple similar version of this header file
// jdchen

//
// input buffer sizes:
// do_prefilter_norm -
// do_stereo_y       - (yim + yim + yim*dlen)*2 + yim*dlen*xwin + 6*64
//                     ~ yim*dlen*(xwin+4)
// do_stereo_d       - (dlen*(yim-YKERN-ywin) + yim + yim*dlen)*2 + yim*dlen*xwin + 6*64
//                     ~ yim*dlen*(xwin+5)

#ifndef STEREOLIB_OSTH
#define STEREOLIB_OSTH

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
  //#ifndef GCC
#include "pstdint.h"		// MSVC++ doesn't have stdint.h
#else
#include <stdint.h>
#endif
#include <math.h>
#include <string.h>
#include <emmintrin.h>
#include <stdlib.h>

// kernel size is fixed
#define KSIZE 7
#define XKERN KSIZE
#define YKERN KSIZE

// filtered disparity value
#define FILTERED -1

// prefilter

#ifdef __SSE2__
#define ost_do_prefilter ost_do_prefilter_fast
#else
#define ost_do_prefilter ost_do_prefilter_norm
#endif

void
ost_do_prefilter_norm(const uint8_t *im,	// input image
	  uint8_t *ftim,	// feature image output
	  int xim, int yim,	// size of image
	  uint8_t ftzero,	// feature offset from zero
	  uint8_t *buf		// buffer storage
	  );

void
ost_do_prefilter_fast(const uint8_t *im,	// input image
	  uint8_t *ftim,	// feature image output
	  int xim, int yim,	// size of image
	  uint8_t ftzero,	// feature offset from zero
	  uint8_t *buf		// buffer storage
	  )
#ifdef GCC
//  __attribute__ ((force_align_arg_pointer)) // align to 16 bytes
#endif
;


#ifdef __cplusplus
}
#endif


#endif


