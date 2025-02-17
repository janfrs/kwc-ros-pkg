//
// C version of stereo algorithm
// Basic column-oriented library
//
// Copyright 2008 by Kurt Konolige
// Videre Design
// kurt@videredesign.com
//

// This file is copied over from stereo/ost. All the functions are
// therefore being put into a namespace of ost
// We may want to consolidate the multiple similar version of this header file
// jdchen


#include "ost_stereolib.h"
#define inline			// use this for Intel Compiler Debug mode

// algorithm requires larger buffers to be passed in
// using lib fns like "memset" can cause problems (?? not sure -
//   it was a stack alignment problem)
// -- but still roll our own

static inline void
memclr(void *buf, int n)
{
  int i;
  unsigned char *bb = (unsigned char *)buf;
  for (i=0; i<n; i++)
    *bb++ = 0;
}


//
// simple normalization prefilter
// XKERN x YKERN mean intensity
// normalize center pixels
// feature values are unsigned bytes, offset at <ftzero>
//
// image aligned at 16 bytes
// feature output aligned at 16 bytes
//

void
ost_do_prefilter_norm(
    const uint8_t *im,	// input image
	  uint8_t *ftim,	// feature image output
	  int xim, int yim,	// size of image
	  uint8_t ftzero,	// feature offset from zero
	  uint8_t *buf		// buffer storage
	  )
{
  int i,j;
  const uint8_t *imp, *impp;
  uint16_t acc;
  const uint8_t *topp, *toppp, *botp, *botpp, *cenp;
  int32_t qval;
  uint32_t uqval;
  uint8_t *ftimp;

  // set up buffers
  uint16_t *accbuf, *accp;
  int ACCBUFSIZE = xim+64;
  accbuf = (uint16_t *)buf;

  // image ptrs
  imp = im;			// leading
  impp = im;			// lagging

  // clear acc buffer
  memclr(accbuf, ACCBUFSIZE*sizeof(int16_t));

  // loop over rows
  for (j=0; j<yim; j++, imp+=xim)
  {
    acc = 0;			// accumulator
    accp = accbuf;		// start at beginning of buf
    topp  = imp;		// leading integration ptr
    toppp = imp;		// lagging integration ptr
    botp  = impp;		// leading integration ptr
    botpp = impp;		// lagging integration ptr

    if (j<YKERN)		// initial row accumulation
    {
      for (i=0; i<XKERN; i++) // initial col accumulation
        acc += *topp++;
      for (; i<xim; i++) // incremental col acc
      {
        acc += *topp++ - *toppp++; // do line sum increment
        *accp++ += acc;	// increment acc buf value
      }
    }
    else			// incremental accumulation
    {
      cenp = topp - (YKERN/2)*xim + XKERN/2; // center pixel
      ftimp = ftim;
      for (i=0; i<XKERN; i++) // initial col accumulation
        acc += *topp++ - *botp++;
      for (; i<xim; i++, accp++, cenp++) // incremental col acc
      {
        acc += *topp++ - *toppp++; // do line sum increment
        acc -= *botp++ - *botpp++; // subtract out previous vals
        *accp += acc;	// increment acc buf value

        // now calculate diff from mean value and save
        qval = 4*(*cenp) + *(cenp-1) + *(cenp+1) + *(cenp-xim) + *(cenp+xim);
#if (XKERN==9)
        qval = (qval*10) - *accp;	// difference with mean, norm is 81
        qval = qval>>4;	// divide by 16 (cenp val divide by ~2)
#endif
#if (XKERN==7)
        qval = (qval*6) - *accp;	// difference with mean, norm is 49
        qval = qval>>3;	// divide by 8 (cenp val divide by ~2)
#endif
        if (qval < -ftzero)
          uqval = 0;
        else if (qval > ftzero)
          uqval = ftzero+ftzero;
        else
          uqval = ftzero - qval;
        *ftimp++ = (uint8_t)uqval;
      }
      impp += xim;		// increment lagging image ptr
      ftim +=  xim;		// increment output ptr
    }
  }
}


#ifdef __SSE2__
// algorithm requires larger buffers to be passed in
// using lib fns like "memset" can cause problems (?? not sure -
//   it was a stack alignment problem)
// -- but still roll our own

static inline void
memclr_si128(__m128i *buf, int n)
{
  int i;
  __m128i zz;
  zz = _mm_setzero_si128();
  n = n>>4;			// divide by 16
  for (i=0; i<n; i++, buf++)
    _mm_store_si128(buf,zz);
}

//
// fast SSE2 version
// NOTE: output buffer <ftim> must be aligned on 16-byte boundary
// NOTE: input image <im> must be aligned on 16-byte boundary
// NOTE: KSIZE (XKERN, YKERN) is fixed at 7
//

#define PXKERN 7
#define PYKERN 7

void
do_prefilter_fast(uint8_t *im,	// input image
	  uint8_t *ftim,	// feature image output
	  int xim, int yim,	// size of image
	  uint8_t ftzero,	// feature offset from zero
	  uint8_t *buf		// buffer storage
	  )
{
  int i,j;

  // set up buffers, first align to 16 bytes
  uintptr_t bufp = (uintptr_t)buf;
  int16_t *accbuf, *accp;
  int16_t *intbuf, *intp;
  uint8_t *imp, *impp, *ftimp;
  __m128i acc, accs, acc1, acct, pxs, opxs, zeros;
  __m128i const_ftzero, const_ftzero_x48, const_ftzero_x2;

  int FACCBUFSIZE = xim+64;

  if (bufp & 0xF)
    bufp = (bufp+15) & ~(uintptr_t)0xF;
  buf = (uint8_t *)bufp;
  accbuf = (int16_t *)buf;

  intbuf = (int16_t *)&buf[FACCBUFSIZE*sizeof(int16_t)];	// integration buffer
  bufp = (uintptr_t)intbuf;
  if (bufp & 0xF)
    bufp = (bufp+15) & ~(uintptr_t)0xF;
  intbuf = (int16_t *)bufp;

  // clear buffers
  memclr_si128((__m128i *)accbuf, FACCBUFSIZE*sizeof(int16_t));
  memclr_si128((__m128i *)intbuf, 8*sizeof(uint16_t));
  impp = im;			// old row window pointer

  // constants
  zeros = _mm_setzero_si128();
  const_ftzero     = _mm_set1_epi16(ftzero);
  const_ftzero_x48 = _mm_set1_epi16(ftzero*48);
  const_ftzero_x2  = _mm_set1_epi16(ftzero*2);

  // loop over rows
  for (j=0; j<yim; j++, im+=xim)
    {
      accp = accbuf;		// start at beginning of buf
      intp = intbuf+8;
      acc = _mm_setzero_si128();
      imp = im;			// new row window ptr
      impp = im - PYKERN*xim;

      // initial row accumulation
      if (j<PYKERN)
	{
	  for (i=0; i<xim; i+=16, imp+=16, intp+=16, accp+=16)
	    {
	      pxs = _mm_load_si128((__m128i *)imp); // next 16 pixels
	      accs = _mm_unpacklo_epi8(pxs,zeros); // unpack first 8 into words
	      acc = _mm_srli_si128(acc, 14); // shift highest word to lowest
	      accs = _mm_add_epi16(accs, acc); // add it in
	      // sum horizontally
	      acct = _mm_slli_si128(accs, 2); // shift left one word
	      accs = _mm_add_epi16(accs, acct); // add it in
	      acct = _mm_slli_si128(accs, 4); // shift left two words
	      accs = _mm_add_epi16(accs, acct); // add it in
	      acct = _mm_slli_si128(accs, 8); // shift left four words
	      acc1 = _mm_add_epi16(accs, acct); // add it in, done
	      _mm_store_si128((__m128i *)intp,acc1); // stored
	      // next 8 pixels
	      accs = _mm_unpackhi_epi8(pxs,zeros); // unpack second 8 into words
	      acc = _mm_srli_si128(acc1, 14); // shift highest word to lowest
	      accs = _mm_add_epi16(accs, acc); // add it in
	      // sum horizontally
	      acct = _mm_slli_si128(accs, 2); // shift left one word
	      accs = _mm_add_epi16(accs, acct); // add it in
	      acct = _mm_slli_si128(accs, 4); // shift left two words
	      accs = _mm_add_epi16(accs, acct); // add it in
	      acct = _mm_slli_si128(accs, 8); // shift left four words
	      acc  = _mm_add_epi16(accs, acct); // add it in, done
	      _mm_store_si128((__m128i *)(intp+8),acc); // stored

	      // update acc buffer, first 8 vals
	      acct = _mm_loadu_si128((__m128i *)(intp-7)); // previous int buffer values
	      acc1 = _mm_sub_epi16(acc1,acct);
	      accs = _mm_load_si128((__m128i *)accp); // acc value
	      acc1 = _mm_add_epi16(acc1,accs);
	      _mm_store_si128((__m128i *)accp,acc1); // stored
	      // update acc buffer, second 8 vals
	      acct = _mm_loadu_si128((__m128i *)(intp-7+8)); // previous int buffer values
	      acc1 = _mm_sub_epi16(acc,acct);
	      accs = _mm_load_si128((__m128i *)(accp+8)); // acc value
	      acc1 = _mm_add_epi16(acc1,accs);
	      _mm_store_si128((__m128i *)(accp+8),acc1); // stored

	    }
	}


      else			// incremental accumulation
	{
	  for (i=0; i<xim; i+=16, imp+=16, impp+=16, intp+=16, accp+=16)
	    {
	      pxs = _mm_load_si128((__m128i *)imp); // next 16 pixels
	      opxs = _mm_load_si128((__m128i *)impp); // next 16 pixels
	      accs = _mm_unpacklo_epi8(pxs,zeros); // unpack first 8 into words
	      acc = _mm_srli_si128(acc, 14); // shift highest word to lowest
	      accs = _mm_add_epi16(accs, acc); // add it in
	      acct = _mm_unpacklo_epi8(opxs,zeros); // unpack first 8 into words
	      accs = _mm_sub_epi16(accs, acct); // subtract it out

	      // sum horizontally
	      acct = _mm_slli_si128(accs, 2); // shift left one word
	      accs = _mm_add_epi16(accs, acct); // add it in
	      acct = _mm_slli_si128(accs, 4); // shift left two words
	      accs = _mm_add_epi16(accs, acct); // add it in
	      acct = _mm_slli_si128(accs, 8); // shift left four words
	      acc1 = _mm_add_epi16(accs, acct); // add it in, done
	      _mm_store_si128((__m128i *)intp,acc1); // stored

	      // next 8 pixels
	      accs = _mm_unpackhi_epi8(pxs,zeros); // unpack second 8 into words
	      acc = _mm_srli_si128(acc1, 14); // shift highest word to lowest
	      accs = _mm_add_epi16(accs, acc); // add it in
	      acct = _mm_unpackhi_epi8(opxs,zeros); // unpack first 8 into words
	      accs = _mm_sub_epi16(accs, acct); // subtract it out

	      // sum horizontally
	      acct = _mm_slli_si128(accs, 2); // shift left one word
	      accs = _mm_add_epi16(accs, acct); // add it in
	      acct = _mm_slli_si128(accs, 4); // shift left two words
	      accs = _mm_add_epi16(accs, acct); // add it in
	      acct = _mm_slli_si128(accs, 8); // shift left four words
	      acc  = _mm_add_epi16(accs, acct); // add it in, done
	      _mm_store_si128((__m128i *)(intp+8),acc); // stored

	      // update acc buffer, first 8 vals
	      acct = _mm_loadu_si128((__m128i *)(intp-7)); // previous int buffer values
	      acc1 = _mm_sub_epi16(acc1,acct);
	      accs = _mm_load_si128((__m128i *)accp); // acc value
	      acc1 = _mm_add_epi16(acc1,accs);
	      _mm_store_si128((__m128i *)accp,acc1); // stored
	      // update acc buffer, second 8 vals
	      acct = _mm_loadu_si128((__m128i *)(intp-7+8)); // previous int buffer values
	      acc1 = _mm_sub_epi16(acc,acct);
	      accs = _mm_load_si128((__m128i *)(accp+8)); // acc value
	      acc1 = _mm_add_epi16(acc1,accs);
	      _mm_store_si128((__m128i *)(accp+8),acc1); // stored
	    }

	  // now do normalization and saving of results
	  ftimp = ftim;
	  accp = accbuf+8;	// start at beginning of good values, off by 1 pixel
	  imp = im - (PYKERN/2)*xim + PXKERN/2 + 1;
	  for (i=0; i<xim-8; i+=16, imp+=16, accp+=16, ftimp+=16)
	    {
	      // sum up weighted pixels in a 4-square pattern
	      pxs = _mm_loadu_si128((__m128i *)imp); // next 16 pixels
	      accs = _mm_unpacklo_epi8(pxs,zeros); // unpack first 8 into words
	      acc  = _mm_unpackhi_epi8(pxs,zeros); // unpack second 8 into words
	      accs = _mm_slli_epi16(accs,2); // multiply by 4
	      acc  = _mm_slli_epi16(acc,2); // multiply by 4
	      pxs  = _mm_loadu_si128((__m128i *)(imp+1)); // 16 pixels to the right
	      acct = _mm_unpacklo_epi8(pxs,zeros); // unpack first 8 into words
	      acc1 = _mm_unpackhi_epi8(pxs,zeros); // unpack second 8 into words
	      accs = _mm_add_epi16(acct,accs);
	      acc  = _mm_add_epi16(acc1,acc);
	      pxs  = _mm_loadu_si128((__m128i *)(imp-1)); // 16 pixels to the left
	      acct = _mm_unpacklo_epi8(pxs,zeros); // unpack first 8 into words
	      acc1 = _mm_unpackhi_epi8(pxs,zeros); // unpack second 8 into words
	      accs = _mm_add_epi16(acct,accs);
	      acc  = _mm_add_epi16(acc1,acc);
	      pxs  = _mm_loadu_si128((__m128i *)(imp+xim)); // 16 pixels below
	      acct = _mm_unpacklo_epi8(pxs,zeros); // unpack first 8 into words
	      acc1 = _mm_unpackhi_epi8(pxs,zeros); // unpack second 8 into words
	      accs = _mm_add_epi16(acct,accs);
	      acc  = _mm_add_epi16(acc1,acc);
	      pxs  = _mm_loadu_si128((__m128i *)(imp-xim)); // 16 pixels above
	      acct = _mm_unpacklo_epi8(pxs,zeros); // unpack first 8 into words
	      acc1 = _mm_unpackhi_epi8(pxs,zeros); // unpack second 8 into words
	      accs = _mm_add_epi16(acct,accs);
	      acc  = _mm_add_epi16(acc1,acc);
	      // first 8 values
	      // times weighted center by 6, giving 48x single pixel value
	      acct = _mm_slli_epi16(accs,2); // multiply by 4
	      accs = _mm_add_epi16(accs,accs); // double
	      accs = _mm_add_epi16(accs,acct); // now x6
	      // subtract from window sum
	      acct = _mm_load_si128((__m128i *)accp);
	      accs = _mm_sub_epi16(accs,acct); // subtract out norm
	      // normalize to ftzero and saturate, divide by 8
	      accs = _mm_srai_epi16(accs,3); // divide by 8
	      accs = _mm_add_epi16(accs,const_ftzero); // normalize to ftzero
	      // saturate to [0, ftzero*2]
	      accs = _mm_max_epi16(accs,zeros);	// floor of 0
	      accs = _mm_min_epi16(accs,const_ftzero_x2);

	      // second 8 values
	      // times weighted center by 6
	      acct = _mm_slli_epi16(acc,2); // multiply by 4
	      acc = _mm_add_epi16(acc,acc); // double
	      acc = _mm_add_epi16(acc,acct); // now x6
	      // subtract from window sum, get absolute value
	      acct = _mm_load_si128((__m128i *)(accp+8));
	      acc1 = _mm_sub_epi16(acc,acct);
	      // normalize to ftzero and saturate, divide by 8
	      acc1 = _mm_srai_epi16(acc1,3); // divide by 8
	      acc1 = _mm_add_epi16(acc1,const_ftzero); // normalize to ftzero
	      // saturate to [0, ftzero*2]
	      acc1 = _mm_max_epi16(acc1,zeros);	// floor of 0
	      acc1 = _mm_min_epi16(acc1,const_ftzero_x2);

	      // pack both results
	      accs = _mm_packs_epi16(accs,acc1);
	      _mm_store_si128((__m128i *)ftimp,accs);

	    }
	  ftim += xim;		// increment destination ptr to next line

	} // end of j >= YKERN section

    }

}

//
// fast version using SSE intrinsics
//


void
do_stereo_d_fast(uint8_t *lim, uint8_t *rim, // input feature images
	    int16_t *disp,	// disparity output
	    int16_t *text,	// texture output
	    int xim, int yim,	// size of images
	    uint8_t ftzero,	// feature offset from zero
	    int xwin, int ywin,	// size of corr window, usually square
	    int dlen,		// size of disparity search, multiple of 8
	    int tfilter_thresh,	// texture filter threshold
	    int ufilter_thresh,	// uniqueness filter threshold, percent
	    uint8_t *buf	// buffer storage
	    )
{
  int i,j,k,d;			// iteration indices
  int16_t *accp, *accpp;	// acc buffer ptrs
  int8_t *limp, *rimp, *limpp, *rimpp, *limp2, *limpp2;	// feature image ptrs
  int8_t *corrend, *corrp, *corrpp; // corr buffer ptrs
  int16_t *intp, *intpp;	// integration buffer pointers
  int16_t *textpp;		// texture buffer pointer
  int16_t *dispp, *disppp;	// disparity output pointer
  int16_t *textp;		// texture output pointer
  int16_t acc;
  int dval;			// disparity value
  int uniqthresh;		// fractional 16-bit threshold

  // xmm variables
  __m128i limpix, rimpix, newpix, tempix, oldpix;
  __m128i newval, temval;
  __m128i minv, indv, indtop, indd, indm, temv, nexv;
  __m128i cval, pval, nval, ival; // correlation values for subpixel disparity interpolation
  __m128i denv, numv, intv, sgnv; // numerator, denominator, interpolation, sign of subpixel interp
  __m128i uniqcnt, uniqth, uniqinit, unim, uval; // uniqueness count, uniqueness threshold

  // xmm constants, remember the 64-bit values are backwards
#ifdef WIN32
  const __m128i p0xfffffff0 = {0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  const __m128i p0x0000000f = {0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			       0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
  const __m128i zeros = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
  const __m128i val_epi16_7fff = {0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f,
				  0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f};
  const __m128i val_epi16_8    = {0x08, 0x0, 0x08, 0x0, 0x08, 0x0, 0x08, 0x0,
				  0x08, 0x0, 0x08, 0x0, 0x08, 0x0, 0x08, 0x0};
  const __m128i val_epi16_1    = {0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
				  0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00};
  const __m128i val_epi16_2    = {0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00,
				  0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00};
#else
  const __m128i p0xfffffff0 = {0xffffffffffff0000LL, 0xffffffffffffffffLL};
  const __m128i p0x0000000f = {0xffffLL, 0x0LL};
  const __m128i zeros = {0x0LL, 0x0LL};
  const __m128i val_epi16_7fff = {0x7fff7fff7fff7fffLL, 0x7fff7fff7fff7fffLL};
  const __m128i val_epi16_8    = {0x0008000800080008LL, 0x0008000800080008LL};
  const __m128i val_epi16_1    = {0x0001000100010001LL, 0x0001000100010001LL};
  const __m128i val_epi16_2    = {0x0002000200020002LL, 0x0002000200020002LL};
#endif

  // set up buffers, first align to 16 bytes
  uintptr_t bufp = (uintptr_t)buf;
  int16_t *intbuf, *textbuf, *accbuf, temp;
  int8_t *corrbuf;

  if (bufp & 0xF)
    bufp = (bufp+15) & ~(uintptr_t)0xF;
  buf = (uint8_t *)bufp;

#define INTEBUFSIZE ((yim+16+ywin)*dlen)
  intbuf  = (int16_t *)buf;	// integration buffer
  bufp = (uintptr_t)intbuf;
  if (bufp & 0xF)
    bufp = (bufp+15) & ~(uintptr_t)0xF;
  intbuf = (int16_t *)bufp;

#define TXTBUFSIZE (yim + 64)
  textbuf = (int16_t *)&buf[INTEBUFSIZE*sizeof(int16_t)];	// texture buffer
  bufp = (uintptr_t)textbuf;
  if (bufp & 0xF)
    bufp = (bufp+15) & ~(uintptr_t)0xF;
  textbuf = (int16_t *)bufp;


#define ACCBUFSIZE (yim*dlen + 64)
  accbuf  = (int16_t *)&buf[(INTEBUFSIZE+TXTBUFSIZE)*sizeof(int16_t)]; // accumulator buffer
  bufp = (uintptr_t)accbuf;
  if (bufp & 0xF)
    bufp = (bufp+15) & ~(uintptr_t)0xF;
  accbuf = (int16_t *)bufp;


  corrbuf = (int8_t *)&buf[(INTEBUFSIZE+TXTBUFSIZE+ACCBUFSIZE)*sizeof(int16_t)]; // correlation buffer
  bufp = (uintptr_t)corrbuf;
  if (bufp & 0xF)
    bufp = (bufp+15) & ~(uintptr_t)0xF;
  corrbuf = (int8_t *)bufp;

  // clear out buffers
  memclr_si128((__m128i *)intbuf, dlen*yim*sizeof(int16_t));
  memclr_si128((__m128i *)corrbuf, dlen*yim*xwin*sizeof(int8_t));
  memclr_si128((__m128i *)accbuf, dlen*yim*sizeof(int16_t));
  memclr_si128((__m128i *)textbuf, yim*sizeof(int16_t));

  // set up corrbuf pointers
  corrend = corrbuf + dlen*yim*xwin;
  corrp = corrbuf;

  // start further out on line to take care of disparity offsets
  limp = (int8_t *)lim + dlen - 1;
  limp2 = limp;
  rimp = (int8_t *)rim;
  dispp = disp + xim*(ywin+YKERN-2)/2 + dlen + (xwin+XKERN-2)/2;
  textp = NULL;
  if (text != NULL)		// optional texture buffer output
    textp = text + dlen;

  // normalize texture threshold
  tfilter_thresh = tfilter_thresh * xwin * ywin * ftzero;
  tfilter_thresh = tfilter_thresh / 100; // now at percent of max
  //  tfilter_thresh = tfilter_thresh / 10;

  // set up some constants
  //  indtop = _mm_set_epi16(dlen+7, dlen+6, dlen+5, dlen+4, dlen+3, dlen+2, dlen+1, dlen);
  indtop = _mm_set_epi16(dlen+0, dlen+1, dlen+2, dlen+3, dlen+4, dlen+5, dlen+6, dlen+7);
  uniqthresh = (0x8000 * ufilter_thresh)/100; // fractional multiplication factor
                                              // for uniqueness test
  uniqinit = _mm_set1_epi16(uniqthresh);
  // init variables so compiler doesn't complain
  intv = zeros;
  ival = zeros;
  nval = zeros;
  pval = zeros;
  cval = zeros;
  uval = zeros;

  // iterate over columns first
  // acc buffer is column-oriented, not line-oriented
  // at each iteration, move across one column
  for (i=0; i<xim-XKERN-dlen+2; i++, limp++, rimp++, corrp+=yim*dlen)
    {
      accp = accbuf;
      if (corrp >= corrend) corrp = corrbuf;
      limpp = limp;
      rimpp = rimp;
      corrpp = corrp;
      intp = intbuf+(ywin-1)*dlen; // intbuf current ptr
      intpp = intbuf;	// intbuf old ptr

      // iterate over rows
      for (j=0; j<yim-YKERN+1; j++, limpp+=xim, rimpp+=xim-dlen)
	{
	  // replicate left image pixel
	  // could replace this with unpacklo_epi8, shufflelo_epi16, pshuffle_epi32
	  limpix = _mm_set1_epi8(*limpp);

	  // iterate over disparities
	  // have to use an intbuf column for each disparity
	  for (d=0; d<dlen; d+=16, intp+=16, intpp+=16, accp+=16, corrpp+=16, rimpp+=16)
	    {
	      // do SAD calculation
	      //	      newc = abs(*limpp - rimpp[d]); // new corr val
	      rimpix = _mm_loadu_si128((__m128i *)rimpp);
	      newpix = _mm_subs_epu8(limpix,rimpix); // subtract pixel values, saturate
	      tempix = _mm_subs_epu8(rimpix,limpix); // subtract pixel values the other way, saturate
	      newpix = _mm_add_epi8(newpix,tempix); // holds abs value
//	      newpix = _mm_sub_epi8(limpix,rimpix); // subtract pixel values, alternate for SSSE3
//	      newpix = _mm_abs_epi8(newpix); // absolute value

	      // calculate new acc values, 8 at a time
	      //	      newv = newc - *corrpp + *intp; // new acc val
	      oldpix = _mm_load_si128((__m128i *)corrpp);
	      newval = _mm_unpacklo_epi8(newpix,zeros); // unpack into words
	      temval = _mm_load_si128((__m128i *)intp);	// get acc values
	      newval = _mm_add_epi16(newval,temval);
	      temval = _mm_unpacklo_epi8(oldpix,zeros); // unpack into words
	      newval = _mm_sub_epi16(newval,temval); // subtract out old corr vals

	      // save new corr value
	      //	      *corrpp++ = newc;	// save new corr val
	      _mm_store_si128((__m128i *)corrpp,newpix);

	      // save new acc values
	      //	      *(intp+dlen) = newv; // save new acc val
	      _mm_store_si128((__m128i *)(intp+dlen),newval);

	      // update windowed sum
	      //	      *accp++ += newv - *intpp++; // update window sum
	      temval = _mm_load_si128((__m128i *)intpp); // get old acc values
	      newval = _mm_sub_epi16(newval,temval);
	      temval = _mm_load_si128((__m128i *)accp); // get window sum values
	      newval = _mm_add_epi16(newval,temval);
	      _mm_store_si128((__m128i *)accp,newval);

	      // next set of 8 values
	      // calculate new acc values, 8 at a time
	      //	      newv = newc - *corrpp + *intp; // new acc val
	      newval = _mm_unpackhi_epi8(newpix,zeros); // unpack into words
	      temval = _mm_load_si128((__m128i *)(intp+8));	// get acc values
	      newval = _mm_add_epi16(newval,temval);
	      temval = _mm_unpackhi_epi8(oldpix,zeros); // unpack into words
	      newval = _mm_sub_epi16(newval,temval); // subtract out old corr vals

	      // save new acc values
	      //	      *(intp+dlen) = newv; // save new acc val
	      _mm_store_si128((__m128i *)(intp+dlen+8),newval);

	      // update windowed sum
	      //	      *accp++ += newv - *intpp++; // update window sum
	      temval = _mm_load_si128((__m128i *)(intpp+8)); // get old acc values
	      newval = _mm_sub_epi16(newval,temval);
	      temval = _mm_load_si128((__m128i *)(accp+8)); // get window sum values
	      newval = _mm_add_epi16(newval,temval);
	      _mm_store_si128((__m128i *)(accp+8),newval);
	    }
	}

      // average texture computation
      // use full corr window
      memclr_si128((__m128i *)intbuf, yim*sizeof(int16_t));
      limpp = limp;
      limpp2 = limp2;
      intp = intbuf+ywin-1;
      intpp = intbuf;
      accpp = textbuf;
      acc = 0;

#if 1				// this should be optimized
      // iterate over rows
      // have to skip down a row each time...
      // check for initial period
      if (i < xwin)
	{
	  for (j=0; j<yim-YKERN+1; j++, limpp+=xim)
	    {
	      temp = abs(*limpp- ftzero);
	      *intp = temp;
	      acc += *intp++ - *intpp++;
	      *accpp++ += acc;
	    }
	}
      else
	{
	  for (j=0; j<yim-YKERN+1; j++, limpp+=xim, limpp2+=xim)
	    {
	      temp = abs(*limpp-ftzero);
	      *intp = temp - abs(*limpp2-ftzero);
	      acc += *intp++ - *intpp++;
	      *accpp++ += acc;
	    }
          limp2++;
	}
#endif

      // disparity extraction, find min of correlations
      if (i >= xwin)		// far enough along...
	{
	  disppp = dispp;
	  accp   = accbuf + (ywin-1)*dlen; // results within initial corr window are partial
	  textpp = textbuf + (ywin-1); // texture measure

	  // start the minimum calc
	  accpp = accp;
	  nexv = val_epi16_7fff;
	  for (d=0; d<dlen; d+=8, accpp+=8)
	    nexv = _mm_min_epi16(nexv,*(__m128i *)accpp); // get window sum values

	  // iterate over rows
	  for (j=0; j<yim-ywin-YKERN+2; j+=8)
	    {
	      //	      if (*textpp < tfilter_thresh && tfilter_thresh > 0)
	      //		{
	      //		  *disppp = FILTERED;
	      //		  continue;
	      //		}

	      intv = _mm_xor_si128(intv,intv); // zero - put here because it crashes gcc-4.1 when below
//	      intv = zeros;

	      // 8 rows at a time
	      for (k=0; k<8; k++, textpp++) // do 8 values at a time
		{
		  // shift all values left 2 bytes for next entry
		  cval = _mm_slli_si128(cval,2);
		  pval = _mm_slli_si128(pval,2);
		  nval = _mm_slli_si128(nval,2);
		  uval = _mm_slli_si128(uval,2);
		  ival = _mm_slli_si128(ival,2);

		  // propagate minimum value
		  // could use _mm_minpos_epu16 if available, still need two shuffles
		  temv = _mm_shufflelo_epi16(nexv,0xb1); // shuffle words
		  temv = _mm_shufflehi_epi16(temv,0xb1); // shuffle words
		  minv = _mm_min_epi16(nexv,temv); // word mins propagated
		  minv = _mm_min_epi16(minv,_mm_shuffle_epi32(minv,0xb1)); // shuffle dwords
		  minv = _mm_min_epi16(minv,_mm_shuffle_epi32(minv,0x4e)); // shuffle dwords
		  // save center value
		  cval = _mm_or_si128(_mm_and_si128(cval,p0xfffffff0),_mm_and_si128(minv,p0x0000000f));
		  // uniqueness threshold
		  uniqth = _mm_mulhi_epu16(uniqinit,minv);
		  uniqth = _mm_add_epi16(uniqth,minv);

		  // find index of min, and uniqueness count
		  // also do next min
		  indv = indtop;
		  indd = val_epi16_8;
		  uniqcnt = zeros;
		  nexv = val_epi16_7fff; // initial min value for next 8 rows
		  for (d=0; d<dlen; d+=8, accp+=8)
		    {
		      indm = _mm_cmpgt_epi16(*(__m128i *)accp,minv); // compare to min
		      indv = _mm_subs_epu16(indv,indd); // decrement indices
		      indd = _mm_and_si128(indd,indm); // wipe out decrement at min
		      unim = _mm_cmpgt_epi16(uniqth,*(__m128i *)accp); // compare to unique thresh
		      nexv = _mm_min_epi16(nexv,*(__m128i *)(accp+dlen)); // get min for next 8 rows
		      uniqcnt = _mm_sub_epi16(uniqcnt,unim); // add in uniq count
		    }
		  indv = _mm_subs_epu16(indv,indd); // decrement indices to saturate at 0
		  // propagate max value
		  temv = _mm_shufflelo_epi16(indv,0xb1); // shuffle words
		  temv = _mm_shufflehi_epi16(temv,0xb1); // shuffle words
		  indv = _mm_max_epi16(indv,temv); // word maxs propagated
		  indv = _mm_max_epi16(indv,_mm_shuffle_epi32(indv,0xb1)); // shuffle dwords
		  indv = _mm_max_epi16(indv,_mm_shuffle_epi32(indv,0x4e)); // shuffle dwords

		  // set up subpixel interpolation (center, previous, next correlation sums)
		  dval = _mm_extract_epi16(indv,0);	// index of minimum
		  nval = _mm_insert_epi16(nval,*(accp-dval),0);
		  pval = _mm_insert_epi16(pval,*(accp-dval-2),0);
		  // save disparity
		  ival = _mm_or_si128(_mm_and_si128(ival,p0xfffffff0),_mm_and_si128(indv,p0x0000000f));

		  // finish up uniqueness count
		  uniqcnt = _mm_sad_epu8(uniqcnt,zeros); // add up each half
		  uniqcnt = _mm_add_epi32(uniqcnt, _mm_srli_si128(uniqcnt,8)); // add two halves
#ifdef EXACTUNIQ
		  unim = _mm_cmpgt_epi16(uniqth,nval); // compare to unique thresh
		  uniqcnt = _mm_add_epi16(uniqcnt,unim); // subtract from uniq count
		  unim = _mm_cmpgt_epi16(uniqth,pval); // compare to unique thresh
		  uniqcnt = _mm_add_epi16(uniqcnt,unim); // subtract from uniq count
#endif
		  if (*textpp < tfilter_thresh)
		    uniqcnt = val_epi16_8; // cancel out this value
		  uval = _mm_or_si128(_mm_and_si128(uval,p0xfffffff0),_mm_and_si128(uniqcnt,p0x0000000f));
		}

	      // disparity interpolation (to 1/16 pixel)
	      // use 16*|p-n| / 2*(p+n-2c)  [p = previous, c = center, n = next]
	      // sign determined by p-n, add increment if positive
	      // denominator is always positive
	      // division done by subtraction and comparison
	      //   computes successive bits of the interpolation
	      // NB: need to check disparities 0 and d-1, skip interpolation

	      // numerator
	      numv = _mm_sub_epi16(pval,nval);
	      sgnv = _mm_cmpgt_epi16(nval,pval); // sign, 0xffff for n>p
	      numv = _mm_xor_si128(numv,sgnv);
	      numv = _mm_sub_epi16(numv,sgnv);	// abs val here

	      // denominator
	      denv = _mm_add_epi16(pval,nval);
	      denv = _mm_sub_epi16(denv,cval);
	      denv = _mm_sub_epi16(denv,cval); // p+n-2c


	      // first bit is always zero unless c = p or c = n, but
	      //   for this case we'll get all 1's in the code below, which will round up
	      // multiply num by 2 for second bit
	      numv = _mm_add_epi16(numv,numv);
	      // second bit
	      temv = _mm_cmpgt_epi16(numv,denv); // 0xffff if n>d
	      intv = _mm_sub_epi16(intv,temv); // add 1 where n>d
	      intv = _mm_slli_epi16(intv,1); // shift left
	      numv = _mm_subs_epu16(numv,_mm_and_si128(temv,denv)); // sub out denominator
	      numv = _mm_slli_epi16(numv,1); // shift left (x2)

	      // third bit
	      temv = _mm_cmpgt_epi16(numv,denv); // 0xffff if n>d
	      intv = _mm_sub_epi16(intv,temv); // add 1 where n>d
	      intv = _mm_slli_epi16(intv,1); // shift left
	      numv = _mm_subs_epu16(numv,_mm_and_si128(temv,denv)); // sub out denominator
	      numv = _mm_slli_epi16(numv,1); // shift left (x2)

	      // fourth bit
	      temv = _mm_cmpgt_epi16(numv,denv); // 0xffff if n>d
	      intv = _mm_sub_epi16(intv,temv); // add 1 where n>d
	      intv = _mm_slli_epi16(intv,1); // shift left
	      numv = _mm_subs_epu16(numv,_mm_and_si128(temv,denv)); // sub out denominator
	      numv = _mm_slli_epi16(numv,1); // shift left (x2)

	      // fifth bit
	      temv = _mm_cmpgt_epi16(numv,denv); // 0xffff if n>d
	      intv = _mm_sub_epi16(intv,temv); // add 1 where n>d

	      // round and do sign
	      intv = _mm_add_epi16(intv,val_epi16_1);	// add in 1 to round
	      intv = _mm_srli_epi16(intv,1);	// shift back
	      intv = _mm_xor_si128(intv,sgnv); // get sign back
	      intv = _mm_sub_epi16(intv,sgnv); // signed val here

	      // add in increment, should be max +-8
	      ival = _mm_slli_epi16(ival,4);
	      ival = _mm_sub_epi16(ival,intv);

	      // check uniq count
	      // null out pval and nval mins
	      //	      unim = _mm_cmpgt_epi16(uniqth,nval); // compare to unique thresh
	      //	      uval = _mm_add_epi16(uval,unim); // sub out
	      //	      unim = _mm_cmpgt_epi16(uniqth,pval); // compare to unique thresh
	      //	      uval = _mm_add_epi16(uval,unim); // sub out
#ifdef EXACTUNIQ
	      uval = _mm_cmpgt_epi16(uval,val_epi16_1);	// 1s where uniqcnt > 1
#else
	      uval = _mm_cmpgt_epi16(uval,val_epi16_2);	// 1s where uniqcnt > 2
#endif
	      ival = _mm_or_si128(uval,ival); // set to -1

	      // store in output array
	      *disppp = _mm_extract_epi16(ival,7);
	      disppp += xim;
	      *disppp = _mm_extract_epi16(ival,6);
	      disppp += xim;
	      *disppp = _mm_extract_epi16(ival,5);
	      disppp += xim;
	      *disppp = _mm_extract_epi16(ival,4);
	      disppp += xim;
	      *disppp = _mm_extract_epi16(ival,3);
	      disppp += xim;
	      *disppp = _mm_extract_epi16(ival,2);
	      disppp += xim;
	      *disppp = _mm_extract_epi16(ival,1);
	      disppp += xim;
	      *disppp = _mm_extract_epi16(ival,0);
	      disppp += xim;

	    } // end of row loop

	  dispp++;		// go to next column of disparity output
	}

    } // end of outer column loop

  // clean up last few rows in case of overrun
  dispp = disp + (yim - YKERN/2 - ywin/2)*xim;
  for (i=0; i<7; i++, dispp+=xim)
    memclr_si128((__m128i *)dispp, xim*sizeof(int16_t));

}
#endif // __SSE2__
