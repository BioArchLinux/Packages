#ifndef PARSIMONY_EVALUATOR
#define PARSIMONY_EVALUATOR

#include "RemoteComm.hpp"
#include <cassert>
#include "TreeAln.hpp"

class ParsimonyEvaluator
{
public:
  ParsimonyEvaluator()
  {}
  
  ParsimonyEvaluator(const ParsimonyEvaluator& rhs )  = default; 
  ParsimonyEvaluator( ParsimonyEvaluator&& rhs ) = default; 
  ParsimonyEvaluator& operator=(const ParsimonyEvaluator &rhs)= default ; 
  ParsimonyEvaluator& operator=( ParsimonyEvaluator &&rhs)= default ; 


  void evaluateSubtree(TreeAln &traln, nodeptr p);

  /** 
      @brief evaluates the parsimony score of the tree

      @param parsimonyLength the per partition parsimony score for the
      transition between the descendent nodes
   */ 
  std::array<parsimonyNumber,3> evaluate(TreeAln &traln, nodeptr p, bool fullTraversal ) ; 
    
  static nat numState2pos(int dataType) 
  { 
    switch(dataType)
      {
      case PLL_BINARY_DATA: 
	return 0 ; 
      case PLL_DNA_DATA: 
	return 1 ; 
      case PLL_AA_DATA: 
	return 2; 
      default: 
	assert(0); 
      }
  }

  static nat pos2numstate(nat pos)
  {
    switch(pos)
      {
      case 0: 
	return PLL_BINARY_DATA; 
      case 1: 
	return PLL_DNA_DATA; 
      case 2: 
	return PLL_AA_DATA; 
      default : 
	assert(0); 
      }
  }

  static void disorientNode(nodeptr p); 
}; 

#endif
