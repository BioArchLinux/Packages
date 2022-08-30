#include <algorithm>
#include <numeric>
#include <vector>

#include "comm/mpi/MpiType.hpp"

template<typename T>
std::vector<T> RemoteComm::Impl::gather(std::vector<T> myData, nat root) 
{
  auto result = std::vector<T>{};  
  nat totalLength = nat(size() * myData.size()); 
  result.resize(totalLength); 

  MPI_Gather( myData.data(), int(myData.size()), mpiType<T>::value, result.data(), int(myData.size()), mpiType<T>::value, root, _comm);

  return result; 
}


// could also return vector<vector<T>>...however, functionality not needed right now 
template<typename T> 
std::vector<T> RemoteComm::Impl::gatherVariableLength(std::vector<T> myData, int root)  
{
  // determine lengths 
  auto lengths = std::vector<nat>{}; 
  auto myLen = std::vector<int>();	
  myLen.push_back(int(myData.size()));
  auto allLengths = gather<int>( myLen, root );
  
  // calculate the displacements 
  auto displ = std::vector<int>{}; 
  displ.push_back(0);
  for(nat i = 1; i < allLengths.size(); ++i)
    displ.push_back(displ.back() + allLengths.at(i-1));

  auto result = std::vector<T>{}; 
  result.resize(std::accumulate(begin(allLengths), end(allLengths), 0));

  MPI_Gatherv(myData.data(), int(myData.size()), mpiType<T>::value, result.data(), allLengths.data(), displ.data(), mpiType<T>::value, root, _comm);
  
  return result; 
} 


template<typename T>
std::vector<T>
RemoteComm::Impl::gatherVariableKnownLength(std::vector<T> myData, std::vector<int> &countsPerProc, std::vector<int> &displPerProc , int root) 
{
  auto myRank = getRank(); 
  auto result = std::vector<T>();

  if( myRank == root )
    result.resize(std::accumulate(begin(countsPerProc), end(countsPerProc), 0),0);
  
  MPI_Gatherv(myData.data(), int(myData.size()),  mpiType<T>::value, result.data(), countsPerProc.data(), displPerProc.data(),mpiType<T>::value,
	      root, _comm); 

  return result; 
}



template<typename T> 
std::vector<T>  
RemoteComm::Impl::scatterVariableKnownLength( std::vector<T> allData, std::vector<int> &countsPerProc, std::vector<int> &displPerProc, int root) 
{
  auto result = std::vector<T>{}; 
  auto myRank = getRank() ; 

  result.resize(countsPerProc.at(myRank)); 

  MPI_Scatterv(myRank == root ? allData.data() : NULL, 
	       countsPerProc.data(), displPerProc.data(), mpiType<T>::value, 
	       result.data(), countsPerProc.at(myRank), mpiType<T>::value, 
	       root, _comm);
  return result; 
} 

template<typename T>
std::vector<T> RemoteComm::Impl::broadcast(std::vector<T> array, int root)  
{
  MPI_Bcast(array.data(), int(array.size()), mpiType<T>::value, root, _comm); 
  return array; 
}

template<typename T>
std::vector<T> RemoteComm::Impl::allReduce( std::vector<T> myValues)
{
  MPI_Allreduce(MPI_IN_PLACE, myValues.data(), int(myValues.size()), mpiType<T>::value, MPI_SUM, _comm); 
  return myValues; 
}

template<typename T> 
T RemoteComm::Impl::receive( int source, int tag ) 
{
  auto result = T{}; 
  MPI_Recv(&result,1, mpiType<T>::value, source, tag, _comm, MPI_STATUS_IGNORE);
  return result; 
}

template<typename T> 
void RemoteComm::Impl::send( T elem, int dest, int tag ) 
{
  MPI_Send(&elem,1, mpiType<T>::value, dest, tag, _comm);
}



