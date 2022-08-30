#ifndef _COMM_REQUEST_IMPL_SEQ_HPP
#define _COMM_REQUEST_IMPL_SEQ_HPP

#include <vector>
#include <memory>

#include "MpiType.hpp"

#include "RemoteComm.hpp"

#include "CommRequest.hpp"

class CommRequest::Impl
{
  friend class RemoteComm; 
  friend class RemoteComm::Impl; 

public: 
  Impl(std::vector<char> array );
  Impl(Impl&& rhs) = delete; 
  Impl(const Impl& rhs) = delete; 
  Impl& operator=(const Impl& rhs) = delete; 
  ~Impl(); 
  std::vector<char> getArray() const; 
  bool isServed() const ; 

private: 
  mutable MPI_Request _req; 
  std::vector<char> _array; 
} ; 


#endif
