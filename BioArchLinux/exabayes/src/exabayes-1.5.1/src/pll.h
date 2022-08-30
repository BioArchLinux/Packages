/**
   @file axml.h
   @brief A wrapper to include all files from pll/examl.   
*/ 


#ifndef _AXML_H_
#define _AXML_H_

#pragma warning  disable 


#include "common.h"


#ifdef _USE_GOOGLE_PROFILER
#include <gperftools/profiler.h>
#endif



/* translate the defines for correct vectorization  */
#if USE_SSE
#define __SSE3
#endif

#if USE_AVX
#define __AVX 
#endif

#ifdef __cplusplus
extern "C"{
#endif

#include "lib/pll/pll-renamed.h"
#include "lib/pll/pllInternal.h"


#ifdef __MIC_NATIVE
#include "lib/pll/mic_native.h"
#endif


#ifdef __cplusplus
}
#endif
/* #pragma warning restore   */


#endif
