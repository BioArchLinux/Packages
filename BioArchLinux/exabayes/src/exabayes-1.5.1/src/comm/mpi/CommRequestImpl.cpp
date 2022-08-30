#include "CommRequestImpl.hpp"

#include  <cassert>

#include "extensions.hpp"

CommRequest::Impl::Impl( std::vector<char> array)
  : _req{MPI_Request{}}
  ,  _array(array)
{
}


CommRequest::Impl::~Impl()
{
  // MPI_Request_free(&_req); 
} 


std::vector<char> CommRequest::Impl::getArray() const 
{
  return _array; 
}


bool CommRequest::Impl::isServed() const 
{
  int flag = 0; 
  MPI_Test(&_req, &flag, MPI_STATUS_IGNORE);
  return flag != 0 ; 
}
