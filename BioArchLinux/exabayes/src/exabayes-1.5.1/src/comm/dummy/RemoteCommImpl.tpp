#include <cassert>

template<typename T>
T RemoteComm::Impl::receive( int source, int tag ) 
{
  assert(0); 
  return 0; 
} 


template<typename T> 
void RemoteComm::Impl::send( T elem, int dest, int tag ) 
{
  assert(0);    
}


template<typename T>
std::vector<T> RemoteComm::Impl::gather(std::vector<T> myData, nat root) 
{
  return myData; 
} 

template<typename T> std::vector<T> RemoteComm::Impl::gatherVariableLength(std::vector<T> myData, int root)  
{
  return myData; 
} 


template<typename T> std::vector<T>  RemoteComm::Impl::scatterVariableKnownLength( std::vector<T> allData, std::vector<int> &countsPerProc, std::vector<int> &displPerProc, int root) 
{
  return allData; 
} 

template<typename T> T RemoteComm::Impl::broadcastVar(T var, int root)  
{
  return var; 
} 


template<typename T> std::vector<T> RemoteComm::Impl::allReduce( std::vector<T> myValues)
{
  return myValues; 
}

template<typename T> 
auto RemoteComm::Impl::broadcast(std::vector<T> array, int root) 
  -> std::vector<T> 
{
  return array; 
} 

template<typename T>
auto RemoteComm::Impl::gatherVariableKnownLength(std::vector<T> myData, std::vector<int> &countsPerProc, std::vector<int> &displPerProc , int root) 
  -> std::vector<T>
{
  return myData; 
}
