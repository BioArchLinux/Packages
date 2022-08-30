#include "FullCachePolicy.hpp"
#include "BranchPlain.hpp"


FullCachePolicy::FullCachePolicy(const TreeAln& traln, bool cacheTipTip, bool cacheTipInner  )
  : restorer(traln, cacheTipTip, cacheTipInner)
{
}


void FullCachePolicy::imprintPolicy(const TreeAln &traln, ArrayOrientation &arrayOrient)  
{
  restorer.resetRestorer(traln, arrayOrient);   
}


void FullCachePolicy::freeMemory(ArrayReservoir& res) 
{
  restorer.clearMemory(res);
}

void FullCachePolicy::accountForRejectionPolicy(TreeAln &traln, const std::vector<bool> &partitions, const std::vector<nat>& invalidNodes, ArrayOrientation &arrayOrient, ArrayReservoir &res)
{
  restorer.restoreSomePartitions(traln, partitions, arrayOrient, res); 
}


std::unique_ptr<ArrayPolicy> FullCachePolicy::clone() const  
{
  return std::unique_ptr<ArrayPolicy>(new FullCachePolicy(*this) ); 
}

void FullCachePolicy::prepareForEvaluation(TreeAln &traln, BranchPlain virtualRoot, nat model, ArrayOrientation& arrayOrientation, ArrayReservoir& res )
{
  // fullTraversal parameter only there to indicate whether we have to
  // search when determining whether arrays are oriented correctly

  restorer.traverseAndCache(traln, traln.findNodePtr(virtualRoot), model,  arrayOrientation,res);
} 
