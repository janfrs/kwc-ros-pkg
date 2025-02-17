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
  // img and featureImg need to be aligned to 16 byte block for sse
#define ost_do_prefilter(im, ftim, xim, yim, ftzero, buf) \
do { \
  if ((uintptr_t)(im) & (uintptr_t)0xF || (uintptr_t)(ftim) & (uintptr_t)0xF ) { \
    cerr<< "buf not aligned for gradient map"<<endl; \
    ost_do_prefilter_fast_u((im), (ftim), (xim), (yim), (ftzero), (buf));\
  } else { \
  ost_do_prefilter_fast((im), (ftim), (xim), (yim), (ftzero), (buf));\
  }\
} while (0)

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
ost_do_prefilter_fast(const uint8_t *im,  // input image
    uint8_t *ftim,  // feature image output
    int xim, int yim, // size of image
    uint8_t ftzero, // feature offset from zero
    uint8_t *buf    // buffer storage
    )
#ifdef GCC
//  __attribute__ ((force_align_arg_pointer)) // align to 16 bytes
#endif
;

// a version that does not require the buffer to aligned to 16 byte
void
ost_do_prefilter_fast_u(const uint8_t *im,  // input image
    uint8_t *ftim,  // feature image output
    int xim, int yim, // size of image
    uint8_t ftzero, // feature offset from zero
    uint8_t *buf    // buffer storage
);

//
// sparse stereo
// returns disparity value in 1/16 pixel, -1 on failure
// refpat is a 16x16 patch around the feature pixel
//   feature pixel is at coordinate 7,7 within the patch
// rim is the right gradient image
// x,y is the feature pixel coordinate
// other parameters as in do_stereo
//

int
ost_do_stereo_sparse(uint8_t *refpat, uint8_t *rim, // input feature images
    int x, int y,         // position of feature pixel
    int xim, int yim, // size of images
    uint8_t ftzero, // feature offset from zero
    int dlen,   // size of disparity search, multiple of 8
    int tfilter_thresh, // texture filter threshold
    int ufilter_thresh  // uniqueness filter threshold, percent
);


#ifdef __cplusplus
}
#endif


#endif


