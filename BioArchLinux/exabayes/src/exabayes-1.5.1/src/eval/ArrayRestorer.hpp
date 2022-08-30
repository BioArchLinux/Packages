/**
   @file ArrayRestorer.hpp 

   @brief allows to restore the lnl array state before evaluating a proposal 
   
   Note that this doubles the overall memory consumption
*/

#ifndef _LNL_RESTORER_H
#define   _LNL_RESTORER_H

#include "pll.h"
#include "TreeAln.hpp"
#include "PartitionLikelihood.hpp"
#include "ArrayOrientation.hpp"
// #include "Branch.hpp"


class ArrayRestorer
{
public: 
  ArrayRestorer(const TreeAln& traln, bool cacheTipTip, bool cacheTipInner); 
  /** 
      @brief resets all information about switched arrays
  */ 
  void resetRestorer(const TreeAln &traln, ArrayOrientation &curOrient); 
  /** 
      @brief restore arrays for the given list of partitions  
  */ 
  void restoreSomePartitions(TreeAln &traln, const std::vector<bool> &partitions, ArrayOrientation &evalOrientation, ArrayReservoir &res); 
  /** 
      @brief traverses a subtree and switches arrays, where necessary 
      @param virtualRoot the root of the subtree 
  */ 
  void traverseAndCache(TreeAln &traln, nodeptr virtualRoot, nat model, ArrayOrientation& arrayOrientation, ArrayReservoir& reservoir); 

  void cache( TreeAln &traln, nat nodeNumber, nat partitionId, const ArrayOrientation &curOrient ); 
  void destroyAndForget(TreeAln &traln, nat nodeNumber, nat partitionId ); 
  void uncache(TreeAln &traln, nat nodeNumber, nat partitionId, ArrayOrientation &curOrient, ArrayReservoir &res  ); 

  void clearMemory(ArrayReservoir &res);
  /** 
      @brief for the sev-memory saving technique, we also need to remember   
   */ 
  void enableRestoreGapVector() { restoresGapVector = true; }  

  void recycleArray(TreeAln& traln, nat nodeNumber, nat partitionId, ArrayReservoir& res); 
  
private: 			// METHODS
  std::pair<double*,nat> removeArray(TreeAln &traln, nat num, nat partition); 
  void insertArray(TreeAln &traln, nat num, nat partition, std::pair<double*,nat> toInsert); 

private: 			// ATTRIBUTES
  std::vector<PartitionLikelihood> partitionLikelihoods; 
  bool restoresGapVector; 
  ArrayOrientation arrayOrientation; 
  bool cacheTipTip; 
  bool cacheTipInner; 
}; 

#endif
