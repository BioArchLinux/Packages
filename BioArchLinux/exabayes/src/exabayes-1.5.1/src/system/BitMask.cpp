#include "BitMask.hpp"

template<typename T> 
BitMask<T>::BitMask()
{
  _impl[0] = 1; 
  for(auto i = 1u; i < length; ++i)
    _impl[i] = _impl[i-1] << 1 ; 
}
template BitMask<unsigned int>::BitMask();

