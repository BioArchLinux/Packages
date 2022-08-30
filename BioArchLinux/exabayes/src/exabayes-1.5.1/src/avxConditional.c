#include "common.h"

#if USE_AVX

#define  __AVX

#include "lib/pll/avxLikelihood.c"

#else 


int avx_prototypeDummy()
{
  return 0; 
}

#endif
