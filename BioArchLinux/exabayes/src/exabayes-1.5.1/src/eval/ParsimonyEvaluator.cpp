#include "ParsimonyEvaluator.hpp"



// not messing around with pll.h any more, let's declare it right here: 
extern "C"
{
  void newviewParsimony(pllInstance *tr, partitionList *pr, nodeptr  p); 
  unsigned int evaluateParsimony(pllInstance *tr, partitionList *pr, nodeptr p, boolean full); 
}


void ParsimonyEvaluator::disorientNode(nodeptr p)
{
  if(p->xPars == 1 )
    {
      p->xPars = 0; 
      p->next->xPars = 1 ; 
      p->next->next->xPars = 0 ; 
    }
}


void ParsimonyEvaluator::evaluateSubtree(TreeAln &traln, nodeptr p)
{
  newviewParsimony(&(traln.getTrHandle()), &(traln.getPartitionsHandle()), p); 
}


std::array<nat,3> ParsimonyEvaluator::evaluate(TreeAln &traln, nodeptr p, bool fullTraversal ) 
{
  auto result = std::array<parsimonyNumber,3>{{0,0,0}}; 

  auto &pHandle = traln.getPartitionsHandle();
  auto &tHandle = traln.getTrHandle();
  
  assert(not tHandle.fastParsimony ); 

  evaluateParsimony(&tHandle, &pHandle, p, fullTraversal ? PLL_TRUE : PLL_FALSE);

  for(nat i = 0; i < traln.getNumberOfPartitions(); ++i)
    {
      auto &partition = traln.getPartition(i); 
      // HACK 
      switch(partition.getDataType())
	{
	case PLL_BINARY_DATA: 
	  result[0] += partition.getPartitionParsimony();
	  break; 
	case PLL_DNA_DATA: 
	  result[1] += partition.getPartitionParsimony(); 
	  break; 
	case PLL_AA_DATA: 
	  result[2] += partition.getPartitionParsimony(); 
	  break; 
	default : 
	  assert(0); 
	}
    }

  return result; 
}
