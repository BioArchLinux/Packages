#include "RemoteCommImpl.hpp"


#include "CommRequest.hpp"
#include <cstdint>
#include <limits>
#include <cassert>

RemoteComm::Impl& RemoteComm::Impl::operator=( Impl rhs)
{
  swap(*this, rhs); 
  return *this;
} 

uint64_t RemoteComm::Impl::_maxTagValue = std::numeric_limits<int>::max();


RemoteComm::Impl RemoteComm::Impl::split(const std::vector<int> &color, const std::vector<int> &rank)  const
{
  auto result = RemoteComm::Impl() ; 
  return result; 
} 


void  RemoteComm::Impl::createSendRequest(std::vector<char> array, int dest, int tag, CommRequest& req)
{
}
 

void RemoteComm::Impl::createRecvRequest(int src, int tag, nat length, CommRequest& req)
{
}  


nat RemoteComm::Impl::getNumberOfPhysicalNodes() 
{
  return 1; 
} 


void RemoteComm::Impl::initComm(int argc, char **argv)
{
  
} 


void RemoteComm::Impl::finalize()
{
  
} 


void RemoteComm::Impl::abort(int code, bool waitForAll)
{
  exit(code); 
} 



template<> bool RemoteComm::Impl::broadcastVar(bool var, int root)  
{
  char elem = var ? 1 : 0; 
  auto res = broadcastVar(elem, root); 
  return res == 1 ; 
}


int RemoteComm::Impl::getRank() const   {return 0 ; } 
size_t RemoteComm::Impl::size() const {return 1u; }  
bool RemoteComm::Impl::isValid() const  {return true; }   


void RemoteComm::Impl::waitAtBarrier()
{
}


bool RemoteComm::Impl::haveThreadSupport() const 
{
  return true; 
}
