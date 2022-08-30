#include "IncompleteMesh.hpp"
#include <cassert>
#include <algorithm>
#include <tuple>

IncompleteMesh::IncompleteMesh(size_t size, size_t runDimSize, size_t chainDimSize)
  : _runDimSize (runDimSize)
  , _chainDimSize ( chainDimSize)
  , _globalSize( size) 
{
}


std::tuple<size_t,size_t> IncompleteMesh::getElementsPerDimension(size_t total, size_t dimSize) const 
{
  auto cartProcs = total  / dimSize; 
  auto remainderProcs = total % dimSize ; 
  auto numMoreProc = remainderProcs; 

  assert(numMoreProc * (cartProcs + 1 )  +  (  dimSize - numMoreProc ) * cartProcs == total); 
  return std::make_tuple(  cartProcs, dimSize - numMoreProc ); 
}


size_t IncompleteMesh::getProcsInMyDim(nat rank, size_t total, size_t dimSize ) const 
{
  auto elemPerDim = getElementsPerDimension(total, dimSize); 
  auto procs = std::get<0>(elemPerDim); 
  auto numWithFewProcs = std::get<1>(elemPerDim); 
  return  ( rank < ( dimSize - numWithFewProcs ) * (procs+1)  )? procs + 1  : procs ; 
}


nat IncompleteMesh::getMyCoord(nat rank, size_t total, size_t dimSize ) const 
{
  auto elemPerDim = getElementsPerDimension(total, dimSize); 
  auto procs = std::get<0>(elemPerDim); 
  auto numWithFew  = std::get<1>(elemPerDim); 
  auto numWithMany = dimSize - numWithFew; 

  if( rank <  numWithMany  * ( procs + 1) )
    return nat(rank / (procs + 1 )); 
  else 
    {
      rank -=  nat(numWithMany * (procs + 1)); 
      return  nat(numWithMany  + rank /  procs); 
    }
}


nat IncompleteMesh::getRankInMyDim(nat rank, size_t total, size_t dimSize) const 
{
  nat procs = 0, numWithFew = 0; 
  std::tie(procs, numWithFew) = getElementsPerDimension(total, dimSize); 
  auto numWithMany = dimSize - numWithFew;  
    
  if(rank < numWithMany * (procs + 1 ))
    return rank % (procs+1); 
  else 
    {
      rank -=  nat(numWithMany * (procs + 1)); 
      return rank % procs; 
    }
}


std::array<nat,3> IncompleteMesh::getCoordinates(nat rank) const
{
  auto result = std::array<nat,3>{{0,0,0}}; 

  result[0] = getMyCoord(rank, _globalSize, _runDimSize); 
    
  auto numProcMyRun = getProcsInMyDim(rank, _globalSize, _runDimSize); 
  auto rankInRun = getRankInMyDim(rank, _globalSize, _runDimSize); 
  result[1] = getMyCoord(rankInRun, numProcMyRun, _chainDimSize); 

  auto numProcMyChain = getProcsInMyDim( rankInRun, numProcMyRun, _chainDimSize ); 
  auto rankInChain = getRankInMyDim(rankInRun, numProcMyRun, _chainDimSize ); 

  result[2] = getMyCoord(rankInChain, numProcMyChain, numProcMyChain); 
    
  return result; 
}


std::ostream& operator<<(std::ostream& out, const IncompleteMesh& rhs)
{
  for(nat i = 0; i < rhs._globalSize; ++i)
    {
      auto coord = rhs.getCoordinates(i); 
      out << "rank:\t" << i << "\t|\t"<< std::get<0>(coord) << "\t" << 
      	std::get<1>(coord) << "\t" << 
      	std::get<2>(coord) <<  std::endl; 
    }
  return out; 
}


nat IncompleteMesh::getRankFromCoordinates( std::array<nat,3> coords) const 
{
  auto result = 0; 
  auto resultRun = 0u; 
  auto resultChain = 0; 

  assert(coords[0] < _runDimSize && coords[1] < _chainDimSize); 
  auto elemPerDim = getElementsPerDimension(_globalSize, _runDimSize); 

  // add ranks for run 
  auto procs = std::get<0>(elemPerDim); 
  auto numWithFew = std::get<1>(elemPerDim);
  auto numWithMany = _runDimSize - numWithFew; 
  if(coords[0] < numWithMany)
    resultRun += nat(coords[0] * (procs + 1 )); 
  else 
    {
      resultRun += nat(numWithMany * (procs + 1)); 
      coords[0] -= nat(numWithMany); 
      resultRun += nat(coords[0] * procs); 
    }

  // add ranks
  auto numProcMyRun = getProcsInMyDim(resultRun, _globalSize, _runDimSize); 
  auto rankInRun = getRankInMyDim(resultRun, _globalSize, _runDimSize); 
  assert(rankInRun == 0); 
  elemPerDim = getElementsPerDimension(numProcMyRun, _chainDimSize); 
  procs = std::get<0>(elemPerDim); 
  numWithFew = std::get<1>(elemPerDim);
  numWithMany = _chainDimSize - numWithFew; 
    
  if(coords[1] < numWithMany)
    resultChain += nat(coords[1] * (procs+1)); 
  else 
    {
      resultChain += nat(numWithMany * (procs+1)); 
      coords[1] -= nat(numWithMany); 
      resultChain += nat(coords[1] *  procs); 
    }
    
  result += resultRun + resultChain + coords[2]; 

  return result; 
}


nat IncompleteMesh::getNumRanksInDim(nat runBatchId, nat chainBatchId) const 
{
  assert(runBatchId < _runDimSize); 
  assert(chainBatchId < _chainDimSize); 
  
  auto baseRank = getRankFromCoordinates( {{ runBatchId, chainBatchId, 0 }}); 
  nat otherRank = 0;  
  
  if(chainBatchId + 1 < _chainDimSize)
    {
      otherRank = getRankFromCoordinates( {{ runBatchId, chainBatchId + 1 , 0 }}); 
    }
  else if(runBatchId + 1 < _runDimSize)
    {
      otherRank = getRankFromCoordinates( {{ runBatchId + 1 , 0 , 0}} ); 
    }
  else 				
    {
      otherRank = nat(_globalSize); 
    }

  assert(baseRank  < otherRank); 

  return otherRank - baseRank; 
}   
