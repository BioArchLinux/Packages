#include "RemoteComm.hpp"
#include "extensions.hpp"

#include "RemoteCommImpl.hpp"
#include "CommRequest.hpp"

using namespace std;

RemoteComm::RemoteComm()
  : _impl{make_unique<RemoteComm::Impl>()}
{
}

RemoteComm::RemoteComm(const RemoteComm &rhs)   
  : _impl(new RemoteComm::Impl(* rhs._impl))
{
} 



RemoteComm::RemoteComm( RemoteComm &&rhs) 
  :_impl(std::move(rhs._impl))
{
}

RemoteComm& RemoteComm::operator=( RemoteComm rhs)
{
  swap(rhs,*this); 
  return *this; 
} 


void swap(RemoteComm &lhs, RemoteComm& rhs)
{
  std::swap(rhs._impl, lhs._impl); 
} 

RemoteComm::~RemoteComm()
{
}


void RemoteComm::waitAtBarrier() const
{
  _impl->waitAtBarrier();
} 

int RemoteComm::getRank() const 
{
  return _impl->getRank();
}
 
size_t RemoteComm::size() const  
{
  return _impl->size(); 
}


bool RemoteComm::isValid() const
{
  return _impl->isValid(); 
} 



template<typename T> 
std::vector<T> RemoteComm::broadcast(std::vector<T> array, int root) 
{
  return _impl->broadcast(array, root);
} 
template auto RemoteComm::broadcast(std::vector<int> array, int root)  -> std::vector<int>; 
template auto RemoteComm::broadcast(std::vector<unsigned int> array, int root)  -> std::vector<unsigned int>; 
// template auto RemoteComm::broadcast(std::vector<unsigned long> array, int root)  -> std::vector<unsigned long>; 


template<typename T> 
T RemoteComm::broadcastVar(T elem  , int root) 
{
  return _impl->broadcastVar(elem, root); 
} 
template uint64_t RemoteComm::broadcastVar(uint64_t var, int root) ; 
template uint32_t RemoteComm::broadcastVar(uint32_t var, int root) ; 
// template bool RemoteComm::broadcastVar(bool var, int root) ; 

template<typename T> 
std::vector<T> RemoteComm::gather(std::vector<T> myData, nat root) 
{
  return _impl->gather(myData, root);
}
template std::vector<char> RemoteComm::gather(std::vector<char> myData, nat root) ; 
template std::vector<int> RemoteComm::gather(std::vector<int> myData, nat root) ; 


template<typename T> 
std::vector<T> RemoteComm::gatherVariableLength(std::vector<T> myData, int root)  
{
  return _impl->gatherVariableLength(myData,root); 
} 
template std::vector<char> RemoteComm::gatherVariableLength(std::vector<char> myData, int root)  ; 
  
template<typename T> 
T RemoteComm::receive( int source, int tag )
{
  return _impl->receive<T>(source, tag) ; 
}
template int RemoteComm::receive(int source , int tag); 
 
template<typename T> 
void RemoteComm::send( T elem, int dest, int tag ) 
{
  return _impl->send(elem, dest, tag); 
} 
template void RemoteComm::send( int elem, int dest, int tag ) ; 



template<typename T> std::vector<T> RemoteComm::allReduce( std::vector<T> myValues)
{
  return _impl->allReduce(myValues); 
} 
template std::vector<double> RemoteComm::allReduce( std::vector<double> myValues); 
template std::vector<uint32_t> RemoteComm::allReduce( std::vector<uint32_t> myValues); 
template std::vector<uint8_t> RemoteComm::allReduce( std::vector<uint8_t> myValues); 





template<typename T> 
auto RemoteComm::scatterVariableKnownLength( std::vector<T> allData, std::vector<int> &countsPerProc, std::vector<int> &displPerProc, int root)  
  -> std::vector<T>
{
  return _impl->scatterVariableKnownLength(allData, countsPerProc, displPerProc, root);
} 
template auto RemoteComm::scatterVariableKnownLength( std::vector<int> allData, std::vector<int> &countsPerProc, std::vector<int> &displPerProc, int root)  -> std::vector<int>; 
template auto RemoteComm::scatterVariableKnownLength( std::vector<uint32_t> allData, std::vector<int> &countsPerProc, std::vector<int> &displPerProc, int root)  -> std::vector<uint32_t>; 
template auto RemoteComm::scatterVariableKnownLength( std::vector<uint16_t> allData, std::vector<int> &countsPerProc, std::vector<int> &displPerProc, int root)  -> std::vector<uint16_t >; 
template auto RemoteComm::scatterVariableKnownLength( std::vector<uint8_t> allData, std::vector<int> &countsPerProc, std::vector<int> &displPerProc, int root)  -> std::vector<uint8_t >; 


RemoteComm RemoteComm::split(const std::vector<int> &color, const std::vector<int> &rank) const 
{
  auto result = RemoteComm();
  *result._impl = _impl->split(color, rank); 
  return result; 
} 


void RemoteComm::createSendRequest(std::vector<char> array, int dest, int tag, CommRequest& req)
{
  _impl->createSendRequest(array, dest, tag, req); 
}

void RemoteComm::createRecvRequest(int src, int tag, nat length, CommRequest& req)
{
  _impl->createRecvRequest(src,tag, length, req); 
}

nat RemoteComm::getNumberOfPhysicalNodes()   
{
  return _impl->getNumberOfPhysicalNodes();
} 


void RemoteComm::finalize()
{
  RemoteComm::Impl::finalize();
}
  
void RemoteComm::initComm(int argc, char **argv)
{
  RemoteComm::Impl::initComm(argc, argv);
}


uint64_t RemoteComm::getMaxTag() 
{
  return  RemoteComm::Impl::_maxTagValue;
}

void RemoteComm::abort(int code, bool waitForAll)
{
  RemoteComm::Impl::abort(code, waitForAll); 
} 


template<typename T>
auto RemoteComm::gatherVariableKnownLength(std::vector<T> myData, std::vector<int> &countsPerProc, std::vector<int> &displPerProc , int root) 
  -> std::vector<T>
{
  return _impl->gatherVariableKnownLength(myData, countsPerProc, displPerProc, root);
}


template auto RemoteComm::gatherVariableKnownLength(std::vector<char> myData, std::vector<int> &countsPerProc, std::vector<int> &displPerProc , int root) -> std::vector<char>; 


void RemoteComm::waitAtBarrier()
{
  _impl->waitAtBarrier();
}


bool RemoteComm::haveThreadSupport() const 
{
  return _impl->haveThreadSupport();
}
