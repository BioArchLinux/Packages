#include "GlobalVariables.hpp"



template<typename T> 
std::vector<T>
Communicator::broadcast(std::vector<T> array, int root) 
{
  auto result = std::vector<T>(array.size()); 
  auto localRoot = mapToLocalRank(root); 
  auto remoteRoot = mapToRemoteRank(root); 

  if(_localComm.getRank() == localRoot)
    result = _remoteComm.broadcast(array, remoteRoot ); 

  assert(remoteRoot >= 0 ); 
  result =_localComm.broadcast(result, remoteRoot); 

  return result; 
}



template<typename T> 
T Communicator::broadcastVar(T elem  , int root) 
{
  auto arr = std::vector<T>  {elem}; 
  arr = broadcast(arr,root); 
  assert(arr.size() == 1); 
  return arr[0]; 
} 


template<typename T> 
std::vector<T> Communicator::gather(std::vector<T> myData, nat root ) 
{
  assert(0); 
  return _remoteComm.gather(myData, root); 
} 


template<typename T> 
std::vector<T>  Communicator::gatherVariableLength(std::vector<T> myData, int root )  
{
  auto localRoot = mapToLocalRank(root); 
  auto remoteRoot = mapToRemoteRank(root); 

  myData = _localComm.gatherVariableLength(myData, localRoot); 

  if(_localComm.getRank() == localRoot)
    myData = _remoteComm.gatherVariableLength(myData, remoteRoot ); 
  
  return myData; 
}

 
template<typename T> 
T Communicator::receive( int source, int tag ) 
{ 
  assert(0); 
  return _remoteComm.receive<T>(source,tag); 
}


template<typename T> 
void Communicator::send( T elem, int dest, int tag ) 
{
  assert(0); 
  _remoteComm.send(elem, dest, tag); 
} 




template<typename T> 
std::vector<T>
Communicator::allReduce(std::vector<T> myValues)
{
  // std::cout << SyncOut()<< _localComm.getColor() << "," << _localComm.getRank()  << "before: " << myValues << std::endl; 

  myValues = _localComm.reduce<T>(myValues, 0);

  if(_localComm.getRank() == 0)
    myValues = _remoteComm.allReduce(myValues); 
  
  myValues = _localComm.broadcast(myValues,0); 

  // std::cout << SyncOut()<< _localComm.getColor() << "," << _localComm.getRank()  << "after: " << myValues << std::endl; 

  return myValues; 
} 


template<typename T>
std::vector<T>
Communicator::gatherVariableKnownLength(std::vector<T> myData, std::vector<int> &countsPerProc, std::vector<int> &displPerProc , int root) 
{
  auto localRoot = mapToLocalRank(root) ; 
  auto remoteRoot = mapToRemoteRank(root); 
  
  auto countsLocal = std::vector<int>(); 
  auto displLocal = std::vector<int>();
  for(auto i = _localComm.size() * _remoteComm.getRank() ; i < _localComm.size() * (_remoteComm.getRank()+1); ++i )
    countsLocal.push_back(countsPerProc[i]);
  displLocal.push_back(0);
  for(nat i  =1; i < countsLocal.size(); ++i)
    displLocal.push_back(displLocal.back() + countsPerProc[i-1]); 
  
  myData = _localComm.gatherVariableKnownLength(myData, countsLocal, displLocal, localRoot); 

  auto countsRemote  = std::vector<int>( _remoteComm.size(), 0 ); 
  for(auto i = 0u; i < size() ;++i)
    countsRemote[i / _localComm.getNumThreads() ] += countsPerProc[i]; 
  auto displRemote = std::vector<int>();
  displRemote.push_back(0); 
  for(nat i = 1 ; i < countsRemote.size() ;++i)
    displRemote.push_back( displRemote[i-1] + countsRemote[i-1]); 

  if(localRoot  == _localComm.getRank() )
    myData = _remoteComm.gatherVariableKnownLength(myData, countsRemote, displRemote, remoteRoot); 

  return myData; 
}



template<typename T> 
std::vector<T> 
Communicator::scatterVariableKnownLength( std::vector<T> myData,  std::vector<int> &countsPerProc,  std::vector<int> &displPerProc, int root )  
{
  auto localRoot = mapToLocalRank(root)	; 
  auto remoteRoot = mapToRemoteRank(root); 

  auto countsRemote  = std::vector<int>( _remoteComm.size(), 0 ); 
  for(auto i = 0u; i < size() ;++i)
    countsRemote[i / _localComm.getNumThreads() ] += countsPerProc[i]; 
  auto displRemote = std::vector<int>();
  displRemote.push_back(0); 
  for(nat i = 1 ; i < countsRemote.size() ;++i)
    displRemote.push_back(displRemote[i-1] + countsRemote[i-1]); 

  // that's the data for our mpi process
  if(localRoot == _localComm.getRank())
    myData = _remoteComm.scatterVariableKnownLength(myData, countsRemote, displRemote, remoteRoot); 
  
  auto countsLocal = std::vector<int>(); 
  auto displLocal = std::vector<int>();
  for(auto i = _localComm.size() * _remoteComm.getRank() ; i < _localComm.size() * (_remoteComm.getRank()+1); ++i )
    countsLocal.push_back(countsPerProc[i]);
  displLocal.push_back(0);
  for(nat i  =1; i < countsLocal.size(); ++i)
    displLocal.push_back(displLocal.back() + countsPerProc[i-1]); 

  myData = _localComm.scatterVariableKnownLength(myData, countsLocal, displLocal, localRoot); 

  return myData ; 
} 

