#include "CommRequestImpl.hpp"

#include "extensions.hpp"

CommRequest::Impl::Impl(std::vector<char> array)
  : _array{}
{
}


CommRequest::Impl::~Impl()
{
} 


bool CommRequest::Impl::isServed()
{
  return true; 
} 


std::vector<char> CommRequest::Impl::getArray() const
{
  return _array; 
} 
