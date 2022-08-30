#ifndef _COMM_REQUEST_IMPL_SEQ_HPP
#define _COMM_REQUEST_IMPL_SEQ_HPP

#include <vector>

#include "CommRequest.hpp"

class CommRequest::Impl
{
public: 
  Impl(std::vector<char> array); 
  ~Impl(); 
  std::vector<char> getArray() const; 
  bool isServed(); 

private: 
  std::vector<char> _array; 
} ; 


#endif
