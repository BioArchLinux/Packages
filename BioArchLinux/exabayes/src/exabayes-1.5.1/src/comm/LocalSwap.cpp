#include "LocalSwap.hpp"
#include "ParallelSetup.hpp"


std::vector<char> LocalSwap::getRemoteData() const 
{
  assert(_haveReceived); 
  return _dataReceived; 
}

 
bool LocalSwap:: isFinished()  
{
  // a normal pending swap has to wait here, until the remote
  // processes have consumed his array. this does not apply here
  return true; 
}
  
bool LocalSwap::allHaveReceived(ParallelSetup& pl) 
{
  // auto runid = pl.getRunComm().getLocalComm().getColor();
  auto myId = _swap.getMyId(pl,_runid); 
  auto remoteId = _swap.getRemoteId(pl,_runid);

  auto tag = cantorPair(remoteId, myId); // NOTE: inverse of initialize 
  assert(tag < pl.getMaxTag()); 

  auto &localComm = pl.getChainComm().getLocalComm(); 
  auto runBatch = pl.getRunComm().getLocalComm().getColor();  
  
  if(not _haveReceived)
    {
      auto wasThere = false; 
      std::tie(wasThere, _dataReceived) = localComm.readAsyncMessage<char>( int(tag), runBatch ); 
      if(wasThere)
	_haveReceived = true; 
    }
  
  auto tmp = std::vector<int>{  _haveReceived ? 1 : 0   };
  tmp = localComm.allReduce(tmp); 
  
  return tmp[0] == int(localComm.size()); 
}

 
void LocalSwap::initialize(ParallelSetup& pl, std::vector<char> myChainSer, nat runid)  
{
  _plPtr = &pl; 
  _runid = runid; 
  // post a message, that should be read by all threads belonging to
  // the other chain
  
  auto runBatch = pl.getRunComm().getLocalComm().getColor();  

  auto myId = _swap.getMyId(pl,runid); 
  auto remoteId = _swap.getRemoteId(pl, runid); 
  auto tag = cantorPair(myId, remoteId); 
  assert(tag < pl.getMaxTag()); 

  if(pl.isChainLeader())
    pl.getChainComm().getLocalComm().postAsyncMessage( myChainSer,  int(tag), runBatch );

} 
