#ifndef _MY_VECT_ALIGN_HPP
#define _MY_VECT_ALIGN_HPP


enum class VectAlign : size_t
{
Normal = sizeof(void*),
  SSE    = 16,
  AVX    = 32
  };

#endif
