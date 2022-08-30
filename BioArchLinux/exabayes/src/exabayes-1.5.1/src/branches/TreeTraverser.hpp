#ifndef _TREE_TRAVERSER_HPP
#define _TREE_TRAVERSER_HPP

#include <unordered_map>

#include "LikelihoodEvaluator.hpp"
#include "TreeAln.hpp"
#include <functional>
#include "InsertionResult.hpp"
#include "ParallelSetup.hpp"
#include "BranchLengthOptimizer.hpp"


class EvalPlain
{
public: 
  InsertionResult getResult (LikelihoodEvaluator &eval, TreeAln &traln, const BranchPlain& insertBranch, const BranchPlain& prunedSubtree, const BranchPlain & draggedBranch, const std::vector<AbstractParameter*> &params) const ; 
}; 


class EvalOptDragged
{
public: 
  InsertionResult getResult (LikelihoodEvaluator &eval, TreeAln &traln, const BranchPlain& insertBranch, const BranchPlain& prunedSubtree, const BranchPlain & draggedBranch, const std::vector<AbstractParameter*> &params) const ; 
}; 

class EvalOptThree
{
public: 
  InsertionResult getResult (LikelihoodEvaluator &eval, TreeAln &traln, const BranchPlain& insertBranch, const BranchPlain& prunedSubtree, const BranchPlain & draggedBranch, const std::vector<AbstractParameter*> &params) const ; 
}; 


template<class InsertEval>
class TreeTraverser
{
public: 
  TreeTraverser(  int depth, TreeAln &traln, LikelihoodEvaluator& eval, std::vector<AbstractParameter*>  params, const BranchPlain &rootOfTraversed, const BranchPlain &prunedSubtree); 

  std::vector<InsertionResult> getResult() const { return _result; }
  void traverse() ; 

private : 			// METHODS
  void testInsert( const BranchPlain &insertBranch, BranchLengths floatingBranch,bool isFirst, int curDepth); 
  

private: 			// ATTRIBUTES
  bool _doFirst; 
  int _depth; 
  std::reference_wrapper<TreeAln> _traln; 
  std::reference_wrapper<LikelihoodEvaluator> _eval; 
  std::vector<AbstractParameter*> _params; 
  BranchPlain _rootOfTraversed; 
  BranchPlain _prunedSubtree; 
  std::vector<InsertionResult> _result; 

  static const int _maxIter; 
  
  InsertEval _insertEval; 
}; 


#include  "TreeTraverser.tpp"

#endif

