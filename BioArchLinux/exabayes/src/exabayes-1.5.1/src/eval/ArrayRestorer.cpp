#include <cstring>
#include <algorithm>
#include <cassert>

#include "ArrayRestorer.hpp" 
#include "GlobalVariables.hpp"

#include "ArrayReservoir.hpp"
#include "FlagType.hpp"


ArrayRestorer::ArrayRestorer(const TreeAln& traln, bool _cacheTipTip, bool _cacheTipInner)
  : partitionLikelihoods{}
  , restoresGapVector(false)
  , arrayOrientation(traln)
  , cacheTipTip(_cacheTipTip)
  , cacheTipInner(_cacheTipInner)
{
  bool useSEV = traln.isSaveMemorySEV();

  for(nat i = 0; i < traln.getNumberOfPartitions(); ++i)
    partitionLikelihoods.emplace_back(traln,i, useSEV); 
}


void ArrayRestorer::restoreSomePartitions(TreeAln &traln, const std::vector<bool> &partitions, ArrayOrientation &evalOrientation, ArrayReservoir& res)
{
  for(nat model = 0; model <  traln.getNumberOfPartitions() ;++model)
    {
      nat partitionIndex = model; 
      // tout << "restoring partition "  << model << std::endl; 

      // restore the partition scaler 
      auto& partition = traln.getPartition( partitionIndex).getHandle(); // TODO lazy 

      if(not partitions[model] || partition.width == 0 )
	continue; 

      
      nat ctr = 0; 
      nat lastNode = traln.getNumberOfNodes() + 1; 
      for(nat i = traln.getNumberOfTaxa() + 1  ; i < lastNode ; ++i)
	{
	  if(partitionLikelihoods[partitionIndex].isCached.at(ctr))
	    uncache(traln, i, partitionIndex, evalOrientation, res); 
	  ++ctr; 
	}

      if(restoresGapVector && partition.width > 0 )
	{
	  std::copy(partitionLikelihoods.at(partitionIndex).gapColumn.begin(), 
		    partitionLikelihoods.at(partitionIndex).gapColumn.end(), 
		    partition.gapColumn ); 
	}
    }
}


void ArrayRestorer::cache( TreeAln &traln, nat nodeNumber, nat partitionId, const ArrayOrientation &curOrient )
{
  // tout << "caching node=" << nodeNumber << ", partition=" << partitionId << std::endl; 

  auto id = nodeNumber - ( traln.getNumberOfTaxa() + 1 ) ; 

  if(partitionLikelihoods[partitionId].cachedArrays.at(id) != nullptr)
    {
      assert(0);
      free(partitionLikelihoods[partitionId].cachedArrays.at(id)); 

    }

  auto arrayAndLength = removeArray(traln, nodeNumber, partitionId); 

  partitionLikelihoods[partitionId].cachedArrays[id] = arrayAndLength.first; 
  partitionLikelihoods[partitionId].lengths[id] = arrayAndLength.second; 
  
  auto& partition = traln.getPartition(partitionId).getHandle(); // TODO lazy 
  partitionLikelihoods[partitionId].scaler[nodeNumber] = partition.globalScaler[nodeNumber]; 

  if(restoresGapVector)
    {
      auto vec = partition.gapVector + nodeNumber * partition.gapVectorLength; 
      auto iter = partitionLikelihoods[partitionId].gapVector.begin() + id * partition.gapVectorLength ; 
      std::copy(vec , vec + partition.gapVectorLength, iter ); 
    }
}



void ArrayRestorer::recycleArray(TreeAln& traln, nat nodeNumber, nat partitionId, ArrayReservoir& res)
{
  auto id = nodeNumber - (traln.getNumberOfTaxa() + 1); 
  auto &partition = traln.getPartition(partitionId).getHandle(); 

  if(partition.xSpaceVector[id] != 0 )
    {
      res.deallocate(partition.xVector[ id ]); 
      partition.xVector[id] = nullptr;
      partition.xSpaceVector[id] = 0; 
    }

  auto &elem  = partitionLikelihoods[partitionId]; 
  
  elem.cachedArrays[id] = nullptr; 
  elem.lengths[id] = 0; 

  arrayOrientation.setInvalid(partitionId, id); 
}


void ArrayRestorer::destroyAndForget(TreeAln &traln, nat nodeNumber, nat partitionId )
{
  // TODO let's recycle instead!
  assert(0); 
  
  auto id = nodeNumber - (traln.getNumberOfTaxa() + 1); 
  auto arrayAndLength = removeArray(traln, nodeNumber, partitionId);
  free(arrayAndLength.first); 
  
  auto &elem  = partitionLikelihoods[partitionId];
  
  elem.cachedArrays[id] = nullptr; 
  elem.lengths[id] = 0; 

  arrayOrientation.setInvalid(partitionId, id);
}


void ArrayRestorer::uncache(TreeAln &traln, nat nodeNumber, nat partitionId, ArrayOrientation &curOrient, ArrayReservoir &res  )
{
  // tout << "uncaching node=" << nodeNumber << ", partition=" << partitionId << std::endl; 

  auto& partition = traln.getPartition(partitionId).getHandle(); 
  auto id = nodeNumber - ( traln.getNumberOfTaxa() + 1) ; 

  auto &backup = partitionLikelihoods[partitionId]; 
  auto arrayAndLength = std::make_pair(backup.cachedArrays.at(id) , 
				       backup.lengths.at(id)); 

  if(partition.xVector[id] != NULL)
    {
      res.deallocate(partition.xVector[id]);
      partition.xVector[id] = nullptr; 
      partition.xSpaceVector[id] = 0; 
    }

  insertArray(traln, nodeNumber, partitionId, arrayAndLength); 
  
  backup.cachedArrays.at(id) = nullptr; 
  backup.lengths.at(id) = 0; 
  
  // backup orientation 
  auto tmp = arrayOrientation.getOrientation(partitionId, id);
  curOrient.setOrientation(partitionId, id, tmp);

  partition.globalScaler[nodeNumber] = backup.scaler[nodeNumber]  ; 
  
  if(restoresGapVector)
    {
      auto vec = partition.gapVector + nodeNumber * partition.gapVectorLength; 
      auto iter = backup.gapVector.begin() + id * partition.gapVectorLength; 
      std::copy(iter, iter + partition.gapVectorLength, vec); 
    }
}


void ArrayRestorer::traverseAndCache(TreeAln &traln, nodeptr virtualRoot, nat model, ArrayOrientation& curOrient, ArrayReservoir& reservoir )
{  
  if(traln.isTipNode(virtualRoot))
    return; 

  nat index = virtualRoot->number - 1 - traln.getNumberOfTaxa(); 

// #if 1
  bool incorrect = not curOrient.isCorrectNew(traln, model, virtualRoot);
// #else 
//   bool incorrect = not curOrient.isCorrect(model, index, virtualRoot->back->number);
// #endif

  auto pnb = virtualRoot->next->back, 
    pnnb = virtualRoot->next->next->back;

  if( incorrect && not partitionLikelihoods[model].isCached[index] ) 
    {
      bool tiptip = traln.isTipNode(pnb) &&  traln.isTipNode(pnnb); 
      bool tipinner = ( not traln.isTipNode(pnb) )   !=  (   not traln.isTipNode(pnnb) ) ; 
      
      bool innerinner = not tiptip && not tipinner;   

      partitionLikelihoods.at(model).isCached[index] = true; 
	  
      if( ( tiptip && cacheTipTip ) 
	  || ( tipinner && cacheTipInner ) 
	  || innerinner )
	{
	  cache(traln, virtualRoot->number , model, curOrient); 
	}
      else 
	{
#if 1 
	  recycleArray( traln, virtualRoot->number, model, reservoir);
#else 
	  destroyAndForget(traln,virtualRoot->number, model);
#endif
	}
    }

  if(incorrect)
    {
      traverseAndCache(traln, pnb, model, curOrient, reservoir); 
      traverseAndCache(traln, pnnb, model, curOrient, reservoir); 
    }
}


void ArrayRestorer::resetRestorer(const TreeAln &traln, ArrayOrientation &curOrient)
{
  for(auto &p : partitionLikelihoods)
    p.isCached = std::vector<bool>(traln.getNumberOfInnerNodes(false), false); 
  
  arrayOrientation = curOrient; 

  for(nat i = 0; i < traln.getNumberOfPartitions(); ++i)
    {
      auto& partition = traln.getPartition( i).getHandle(); 

      if( restoresGapVector && partition.width > 0  )
	{
	  // problematic? 
	  std::copy(partition.gapColumn ,
		    partition.gapColumn + traln.getNumberOfTaxa() * partition.states * 4, 
		    partitionLikelihoods.at(i).gapColumn.begin() ); 
	}
    }
}


void ArrayRestorer::clearMemory(ArrayReservoir &res)
{
  for(auto &partitionLikelihood : partitionLikelihoods)
    {
      for (nat i = 0; i < partitionLikelihood.cachedArrays.size();  ++i)
	{
	  if(partitionLikelihood.cachedArrays.at(i) != nullptr)
	    {
	      res.deallocate(partitionLikelihood.cachedArrays.at(i)); 
	      partitionLikelihood.cachedArrays.at(i) = nullptr; 
	      partitionLikelihood.lengths.at(i) = 0; 
	    }
	}
    }
}


std::pair<double*,nat> ArrayRestorer::removeArray(TreeAln &traln, nat num, nat pid)
{
  auto& partition = traln.getPartition(pid).getHandle();

  nat id = num - traln.getNumberOfTaxa() - 1; 

  double *array = partition.xVector[id];
  auto length = partition.xSpaceVector[id]; 
  
  partition.xVector[id] = NULL; 
  partition.xSpaceVector[id] = 0; 

  return std::make_pair(array, length); 
}


void ArrayRestorer::insertArray(TreeAln &traln, nat num, nat pid, std::pair<double*,nat> toInsert)
{
  auto& partition = traln.getPartition(pid).getHandle();
  nat id = num - traln.getNumberOfTaxa() - 1 ; 

  assert(partition.xVector[id] == NULL); 
  
  partition.xVector[id] = toInsert.first; 
  partition.xSpaceVector[id] = toInsert.second; 
}
