#include "Communicator.hpp"	 
#include "GlobalVariables.hpp"
#include "ThreadResource.hpp"
#include "AbstractPendingSwap.hpp"

Communicator::Communicator(std::unordered_map<tid_t,int> tid2rank)
  : _remoteComm()
  , _localComm(tid2rank)
{
  auto tRanks =  std::vector<int> {}; 
  for(nat i =0; i < tid2rank.size() ;++i)
    tRanks.push_back(i); 
  _localComm.setRanks(tRanks); 
}


Communicator& Communicator::operator=(Communicator rhs) 
{
  swap(*this, rhs); 
  return *this; 
}


void swap(Communicator& lhs, Communicator &rhs)
{
  using std::swap; 
  swap(lhs._remoteComm, rhs._remoteComm); 
  swap(lhs._localComm, rhs._localComm); 
}


void Communicator::waitAtBarrier() 
{
  if(_localComm.getRank() == 0)
    _remoteComm.waitAtBarrier();

  _localComm.waitAtBarrier();
}


bool Communicator::haveThreadSupport() const 
{
  return _localComm.haveThreadSupport() && _remoteComm.haveThreadSupport(); 
}

void Communicator::createSendRequest(std::vector<char> array, int dest, int tag, CommRequest& req)
{
  _remoteComm.createSendRequest(array, dest,tag, req); 
}

void Communicator::createRecvRequest(int src, int tag, nat length, CommRequest& req)
{
  _remoteComm.createRecvRequest(src,tag, length, req); 
}


int Communicator::getRank( ) const 
{
  return _remoteComm.getRank()  * int(_localComm.size()) + _localComm.getRank() ; 
}

size_t Communicator::size() const 
{
  return _remoteComm.size() * _localComm.size(); 
}

bool Communicator::isValid() const
{
  return _remoteComm.isValid() && _localComm.isValid();
}

Communicator Communicator::split(const std::vector<int> &color, const std::vector<int> &rank) const 
{
  // std::cout << "called split with " << color << " and "  << rank << std::endl; 

  auto &&result = Communicator(_localComm.getTid2Ranking()) ; 

  result._remoteComm = _remoteComm.split(color, rank);
  result._localComm = std::move(_localComm.split(color, rank)); 

  return std::move(result); 
}


void Communicator::finalize()
{
  RemoteComm::finalize(); 
}

void Communicator::initComm(int argc, char **argv)
{
  RemoteComm::initComm(argc, argv); 
}


#include <unistd.h>

void Communicator::abort(int code, bool waitForAll)
{
  LocalComm::abort(code,waitForAll); 
  RemoteComm::abort(code, waitForAll); 

  exit(code); 
  // exit(code); 
}


std::ostream& operator<<(std::ostream & out, const Communicator& rhs)
{
  out << rhs._remoteComm << "\t" ; 
  out << rhs._localComm ; 
  return out; 
}


int Communicator::mapToLocalRank( int rank) const   
{
  auto res =  rank % int(_localComm.size()); 
  assert(res >= 0 && "local rank was negative" ); 
  return  res; 
}


int Communicator::mapToRemoteRank(int rank) const  
{
  return rank /int( _localComm.size()); 
}


/** 
    how many sets of threads offset does this set of threads need for
    pinning?
 */ 
int Communicator::getProcsPerNode() 
{
  auto numNodes = _remoteComm.getNumberOfPhysicalNodes(); 
  auto siz = _remoteComm.size(); 
  auto procsPerNode = (siz / numNodes) + (siz % numNodes == 0 ? 0 : 1 ) ; 
  return int(procsPerNode); 
}


LocalComm&  Communicator::getLocalComm ()
{
  return _localComm;
} 


RemoteComm& Communicator::getRemoteComm() 
{
  return _remoteComm; 
}


void Communicator::initWithMaxChains(size_t numChains, size_t numThreadsChecking)
{
  if(_localComm.getColor() == 0 && _localComm.getRank() == 0)
    _localComm.initializeAsyncQueue(nat(numThreadsChecking), size_t(AbstractPendingSwap::cantorPair(numChains, numChains) * 2) );
} 
