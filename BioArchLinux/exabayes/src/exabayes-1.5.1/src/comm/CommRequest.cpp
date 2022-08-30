#include "CommRequest.hpp"

#include <cassert>
#include <iostream>

#include "RemoteComm.hpp"
#include "extensions.hpp"
#include "CommRequestImpl.hpp"

using namespace std;

CommRequest::CommRequest( std::vector<char> array  )
  : _impl( make_unique<CommRequest::Impl>(array) )
{
  
}


CommRequest::CommRequest(CommRequest &&rhs)   
  : _impl{std::move(rhs._impl)}
{
  
}


CommRequest::~CommRequest()
{
} 


bool CommRequest::isServed() const 
{
  return _impl->isServed(); 
}


std::vector<char> CommRequest::getArray() const 
{
  return _impl->getArray();
}


void  swap(CommRequest& lhs, CommRequest& rhs)
{
  std::swap(lhs._impl, rhs._impl);
} 


CommRequest& CommRequest::operator=(CommRequest rhs)
{
  swap(rhs, *this); 
  return *this; 
} 
