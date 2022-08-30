#ifndef FLAG_TYPE
#define FLAG_TYPE

#include <type_traits>


// icc wants prior definition 
template<typename T> T operator|( T a, T b)  ; 
template<typename T> T operator&( T a,  T b) ; 
template<typename T> T operator&=(T a, T b) ; 
template<typename T> T operator|=(T a, T b) ;  


template<typename T>
T operator|( T a, T b) 
{
  static_assert(std::is_enum<T>::value  , "type must be an enum!\n"); 
  return static_cast<T>(static_cast<int>(a) | static_cast<int>(b)); 
}

template<typename T>
T operator&( T a,  T b)
{
  static_assert(std::is_enum<T>::value, "type must be an enum!\n"); 
  return static_cast<T>(static_cast<int>(a) & static_cast<int>(b)); 
}


template<typename T>
T operator&=(T a, T b)
{
  return a & b; 
}


template<typename T>
T operator|=(T a, T b)
{
  return a | b; 
}



#endif
